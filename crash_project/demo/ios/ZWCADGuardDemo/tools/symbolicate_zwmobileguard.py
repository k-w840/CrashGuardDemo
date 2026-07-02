#!/usr/bin/env python3
"""
根据当前 ZWMobileGuard 落地的 crash json，批量调用 atos 做离线符号化。

当前 SDK 日志格式特点：
1. binary_images 中有 image_addr / image_size / uuid / name
2. 栈帧地址在 report.crash.threads[*].backtrace.contents[*].instruction_addr
3. 当前日志未落 image path，所以主程序可以自动解，其他 image 需要手工补名字到符号文件路径的映射

常见用法：
    python3 tools/symbolicate_zwmobileguard.py \
      --report /path/to/crash_123.json \
      --main-dsym /path/to/ZWCADGuardDemo.app.dSYM

    python3 tools/symbolicate_zwmobileguard.py \
      --report /path/to/crash_123.json \
      --main-binary /path/to/ZWCADGuardDemo.app/ZWCADGuardDemo \
      --image MyFramework=/path/to/MyFramework.framework.dSYM \
      --output /tmp/symbolicated.txt
"""

from __future__ import annotations

import argparse
import datetime as _dt
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from typing import Dict, List, Optional, Sequence, Tuple


HEX_NUMBER_RE = re.compile(r'(?P<prefix>[:\[,]\s*)(?P<hex>0x[0-9a-fA-F]+)(?P<suffix>\s*[,}\]])')


@dataclass
class BinaryImage:
    name: str
    path: str
    base: int
    size: int
    uuid: str

    @property
    def end(self) -> int:
        return self.base + self.size


@dataclass
class Frame:
    index: int
    address: int
    image: Optional[BinaryImage]
    atos_address: int
    symbol: Optional[str] = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Symbolicate ZWMobileGuard crash report with atos.")
    parser.add_argument("--report", required=True, help="ZWMobileGuard 落地的 crash json 文件路径")
    parser.add_argument("--main-binary", help="主程序可执行文件路径，例如 ZWCAD.app/ZWCAD")
    parser.add_argument("--main-dsym", help="主程序 dSYM 路径，例如 ZWCAD.app.dSYM")
    parser.add_argument(
        "--image",
        action="append",
        default=[],
        help="额外 image 符号文件映射，格式 Name=/path/to/binary_or_dsym，可重复传入",
    )
    parser.add_argument("--arch", default="arm64", help="atos 使用的架构，默认 arm64")
    parser.add_argument("--thread-index", type=int, default=0, help="要符号化的线程索引，默认 0")
    parser.add_argument("--output", help="输出文件路径；不传则打印到标准输出")
    parser.add_argument(
        "--show-registers",
        action="store_true",
        help="在输出里附带 basic/exception registers",
    )
    return parser.parse_args()


def load_report(report_path: str) -> dict:
    with open(report_path, "r", encoding="utf-8") as fp:
        content = fp.read()

    try:
        return json.loads(content)
    except json.JSONDecodeError:
        # 兼容旧版本日志：地址被写成 0x1234 这种 JSON 非法数字时，先转成十进制再解析。
        fixed = HEX_NUMBER_RE.sub(lambda m: f"{m.group('prefix')}{int(m.group('hex'), 16)}{m.group('suffix')}", content)
        return json.loads(fixed)


def normalize_uuid(value: str) -> str:
    return (value or "").strip().upper()


def parse_int_value(value: object, default: int = 0) -> int:
    if value is None:
        return default
    if isinstance(value, str):
        value = value.strip()
        if not value:
            return default
        return int(value, 0)
    return int(value)


def parse_binary_images(report: dict) -> List[BinaryImage]:
    images = []
    raw_images = report.get("report", {}).get("binary_images", [])
    for item in raw_images:
        images.append(
            BinaryImage(
                name=str(item.get("name", "")),
                path=str(item.get("path", "")),
                base=parse_int_value(item.get("image_addr")),
                size=parse_int_value(item.get("image_size")),
                uuid=normalize_uuid(str(item.get("uuid", ""))),
            )
        )
    return images


def normalize_instruction_address(address: int) -> int:
    return address & 0x0000000FFFFFFFFF


def call_instruction_address(address: int, arch: str) -> int:
    normalized = normalize_instruction_address(address)
    arch = (arch or "").lower()
    if arch.startswith("arm64") or arch.startswith("aarch64"):
        normalized &= ~0x3
    elif arch.startswith("arm"):
        normalized &= ~0x1

    if normalized <= 1:
        return normalized
    return normalized - 1


def find_image_for_address(images: Sequence[BinaryImage], address: int) -> Optional[BinaryImage]:
    for image in images:
        if image.base <= address < image.end:
            return image
    return None


def parse_frames(report: dict, images: Sequence[BinaryImage], thread_index: int, arch: str) -> List[Frame]:
    threads = report.get("report", {}).get("crash", {}).get("threads", [])
    if thread_index >= len(threads):
        raise IndexError(f"thread-index={thread_index} 超出范围，当前线程数为 {len(threads)}")

    error_type = report.get("report", {}).get("crash", {}).get("error", {}).get("type", "")
    contents = threads[thread_index].get("backtrace", {}).get("contents", [])
    frames = []
    for idx, item in enumerate(contents):
        address = parse_int_value(item.get("instruction_addr"))
        atos_address = address
        # signal 栈首帧通常就是 faulting PC，后续帧更接近“返回地址”，按 KSCrash 风格转成调用点更容易命中静态库内部符号。
        if not (error_type == "signal" and idx == 0):
            atos_address = call_instruction_address(address, arch)
        frames.append(Frame(index=idx, address=address, image=find_image_for_address(images, address), atos_address=atos_address))
    return frames


def extract_process_name(report: dict) -> str:
    return str(report.get("report", {}).get("report", {}).get("process_name", "")).strip()


def resolve_dsym_executable(dsym_path: str, image_name: str) -> str:
    dwarf_dir = os.path.join(dsym_path, "Contents", "Resources", "DWARF")
    exact = os.path.join(dwarf_dir, image_name)
    if os.path.isfile(exact):
        return exact

    if os.path.isdir(dwarf_dir):
        entries = [os.path.join(dwarf_dir, name) for name in os.listdir(dwarf_dir)]
        files = [path for path in entries if os.path.isfile(path)]
        if len(files) == 1:
            return files[0]

    raise FileNotFoundError(f"无法在 dSYM 中找到 {image_name} 对应的 DWARF 文件: {dsym_path}")


def resolve_symbol_path(path: str, image_name: str) -> str:
    if path.endswith(".dSYM"):
        return resolve_dsym_executable(path, image_name)

    if path.endswith(".app") and os.path.isdir(path):
        candidate = os.path.join(path, image_name)
        if os.path.isfile(candidate):
            return candidate
        raise FileNotFoundError(f"无法在 app bundle 中找到可执行文件: {candidate}")

    return path


def build_symbol_mapping(args: argparse.Namespace, report: dict) -> Dict[str, str]:
    process_name = extract_process_name(report)
    mapping: Dict[str, str] = {}

    if args.main_dsym:
        mapping[process_name] = resolve_symbol_path(args.main_dsym, process_name)
    elif args.main_binary:
        mapping[process_name] = resolve_symbol_path(args.main_binary, process_name)

    if args.main_binary:
        mapping[os.path.basename(args.main_binary)] = resolve_symbol_path(args.main_binary, process_name)

    for item in args.image:
        if "=" not in item:
            raise ValueError(f"--image 参数格式错误，应为 Name=/path/to/file，实际收到: {item}")
        name, raw_path = item.split("=", 1)
        name = name.strip()
        raw_path = raw_path.strip()
        mapping[name] = resolve_symbol_path(raw_path, name)

    return mapping


def run_command(command: Sequence[str]) -> str:
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        stderr = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError(f"命令执行失败: {' '.join(command)}\n{stderr}")
    return result.stdout


def read_uuid_set(target_path: str) -> List[str]:
    try:
        output = run_command(["dwarfdump", "--uuid", target_path])
    except Exception:
        return []

    uuids = []
    for line in output.splitlines():
        match = re.search(r"UUID:\s*([0-9A-Fa-f\-]+)", line)
        if match:
            uuids.append(normalize_uuid(match.group(1)))
    return uuids


def symbolicate_group(arch: str, symbol_path: str, load_address: int, addresses: Sequence[int]) -> Dict[int, str]:
    if not addresses:
        return {}

    command = [
        "xcrun",
        "atos",
        "-arch",
        arch,
        "-o",
        symbol_path,
        "-l",
        hex(load_address),
    ] + [hex(addr) for addr in addresses]

    output = run_command(command)
    lines = [line.strip() for line in output.splitlines()]
    if len(lines) != len(addresses):
        raise RuntimeError(
            f"atos 返回行数与输入地址数不一致: {symbol_path}, 期望 {len(addresses)} 行, 实际 {len(lines)} 行"
        )
    return dict(zip(addresses, lines))


def symbolicate_frames(
    frames: Sequence[Frame],
    mapping: Dict[str, str],
    arch: str,
) -> List[str]:
    notes: List[str] = []
    grouped: Dict[Tuple[str, int], List[int]] = {}
    image_by_group: Dict[Tuple[str, int], BinaryImage] = {}

    for frame in frames:
        if frame.image is None:
            continue
        symbol_path = mapping.get(frame.image.name)
        if not symbol_path:
            continue
        key = (symbol_path, frame.image.base)
        grouped.setdefault(key, [])
        if frame.atos_address not in grouped[key]:
            grouped[key].append(frame.atos_address)
        image_by_group[key] = frame.image

    resolved: Dict[int, str] = {}
    for (symbol_path, load_address), addresses in grouped.items():
        image = image_by_group[(symbol_path, load_address)]
        uuids = read_uuid_set(symbol_path)
        if uuids and image.uuid and image.uuid not in uuids:
            notes.append(
                f"[UUID不匹配] image={image.name} report={image.uuid} file={','.join(uuids)} path={symbol_path}"
            )
        try:
            resolved.update(symbolicate_group(arch, symbol_path, load_address, addresses))
        except Exception as exc:
            notes.append(f"[符号化失败] image={image.name} path={symbol_path} error={exc}")

    for frame in frames:
        if frame.atos_address in resolved:
            frame.symbol = resolved[frame.atos_address]

    return notes


def format_timestamp(report: dict) -> str:
    value = report.get("report", {}).get("report", {}).get("timestamp")
    if value is None:
        return ""
    try:
        timestamp = int(value)
        dt = _dt.datetime.fromtimestamp(timestamp)
        return dt.strftime("%Y-%m-%d %H:%M:%S")
    except Exception:
        return str(value)


def format_address(value: object) -> str:
    return f"0x{parse_int_value(value):x}"


def build_output(report: dict, frames: Sequence[Frame], notes: Sequence[str], show_registers: bool) -> str:
    lines: List[str] = []
    report_info = report.get("report", {}).get("report", {})
    crash_info = report.get("report", {}).get("crash", {})
    error_info = crash_info.get("error", {})

    lines.append(f"Report ID: {report_info.get('id', '')}")
    lines.append(f"Process: {report_info.get('process_name', '')}")
    lines.append(f"Timestamp: {format_timestamp(report)}")
    lines.append(f"Error Type: {error_info.get('type', '')}")
    lines.append("")
    lines.append("Backtrace:")

    for frame in frames:
        image_name = frame.image.name if frame.image else "<unknown>"
        image_path = frame.image.path if frame.image else ""
        symbol = frame.symbol or "<unresolved>"
        address = format_address(frame.address)
        if image_path:
            lines.append(f"{frame.index:02d}  {address}  {image_name}  {symbol}")
            lines.append(f"    path: {image_path}")
        else:
            lines.append(f"{frame.index:02d}  {address}  {image_name}  {symbol}")

    if show_registers:
        thread = crash_info.get("threads", [{}])[0]
        registers = thread.get("registers", {})
        basic = registers.get("basic", {})
        exception = registers.get("exception", {})
        lines.append("")
        lines.append("Registers.basic:")
        for key in ("pc", "sp", "lr", "fp", "cpsr"):
            if key in basic:
                lines.append(f"  {key}: {format_address(basic[key])}")
        x_keys = sorted((key for key in basic.keys() if re.fullmatch(r"x\d+", key)), key=lambda item: int(item[1:]))
        for key in x_keys:
            lines.append(f"  {key}: {format_address(basic[key])}")
        lines.append("Registers.exception:")
        for key in ("esr", "far"):
            if key in exception:
                lines.append(f"  {key}: {format_address(exception[key])}")

    if notes:
        lines.append("")
        lines.append("Notes:")
        lines.extend(f"- {note}" for note in notes)

    lines.append("")
    lines.append("Hint:")
    lines.append("- 当前 SDK 日志未落 image path，未传映射的 image 会保持 unresolved。")
    lines.append("- 主程序通常传 --main-dsym 即可批量解出宿主可执行文件中的静态库代码。")

    return "\n".join(lines)


def main() -> int:
    args = parse_args()

    try:
        report = load_report(args.report)
        images = parse_binary_images(report)
        frames = parse_frames(report, images, args.thread_index, args.arch)
        mapping = build_symbol_mapping(args, report)
        notes = symbolicate_frames(frames, mapping, args.arch)
        output = build_output(report, frames, notes, args.show_registers)
    except Exception as exc:
        print(f"symbolicate_zwmobileguard.py 执行失败: {exc}", file=sys.stderr)
        return 1

    if args.output:
        with open(args.output, "w", encoding="utf-8") as fp:
            fp.write(output)
    else:
        print(output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
