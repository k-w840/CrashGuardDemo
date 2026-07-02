from http.server import HTTPServer, BaseHTTPRequestHandler
import os
import traceback

# 指定接收文件存放的目录
TARGET_DIR = os.path.expanduser("~/Desktop/Uploads")

class CustomHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        try:
            if not os.path.exists(TARGET_DIR):
                os.makedirs(TARGET_DIR)

            # 1. 获取并验证 Content-Type 
            content_type = self.headers.get('Content-Type', '')
            if 'multipart/form-data' not in content_type:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"Content-Type must be multipart/form-data")
                return

            # 2. 从 Header 中精准提取 boundary 文本
            # 兼容 boundary=xxx 以及 boundary="xxx" 的形式
            boundary_str = ""
            for item in content_type.split(';'):
                item = item.strip()
                if item.startswith('boundary='):
                    boundary_str = item.split('boundary=')[1].strip('"')
                    break
            
            if not boundary_str:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"No boundary found in Content-Type")
                return

            # 转换为 HTTP 协议标准的分隔符字节流
            boundary = b"--" + boundary_str.encode('utf-8')

            # 3. 读取完整的 Body 字节流
            content_length = int(self.headers.get('Content-Length', 0))
            body_bytes = self.rfile.read(content_length)

            # 4. 根据 boundary 切分多部分表单
            parts = body_bytes.split(boundary)
            
            for part in parts:
                # 过滤掉空块和结尾块（结尾块通常以 b'--\r\n' 结束）
                if not part or part.startswith(b'--') or b'filename=' not in part:
                    continue
                
                # 拆分 Part 的 Header 和 Body (它们之间由 \r\n\r\n 分隔)
                sub_parts = part.split(b'\r\n\r\n', 1)
                if len(sub_parts) < 2:
                    continue
                
                part_header = sub_parts[0].decode('utf-8', errors='ignore')
                part_body = sub_parts[1]
                
                # 去除末尾的换行符 \r\n
                if part_body.endswith(b'\r\n'):
                    part_body = part_body[:-2]

                # 5. 从 Part Header 中提取文件名
                filename = "uploaded_file.txt"
                for line in part_header.split('\r\n'):
                    if 'Content-Disposition' in line and 'filename=' in line:
                        # 提取 filename="..." 内部的文件名
                        filename = line.split('filename=')[1].strip('"')
                        break

                # 6. 写入到 Mac 指定路径
                filepath = os.path.join(TARGET_DIR, filename)
                with open(filepath, 'wb') as f:
                    f.write(part_body)

                print(f"【成功】文件已安全保存至: {filepath}")
                
                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(b'{"status": "success", "message": "File uploaded successfully"}')
                return

            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Field 'file' missing or empty in multipart form")

        except Exception as e:
            print("--- 服务器内部发生崩溃 ---")
            traceback.print_exc()
            
            self.send_response(500)
            self.end_headers()
            self.wfile.write(f"Internal Server Error: {str(e)}".encode())

if __name__ == '__main__':
    server = HTTPServer(('0.0.0.0', 8080), CustomHTTPRequestHandler)
    print(f"服务已重启。监听端口 :8080，保存目录: {TARGET_DIR}")
    server.serve_forever()