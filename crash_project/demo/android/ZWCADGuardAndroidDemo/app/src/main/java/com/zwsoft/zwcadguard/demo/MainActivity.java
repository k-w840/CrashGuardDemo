package com.zwsoft.zwcadguard.demo;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.zwsoft.zwcadguard.ZWCADGuard;

import java.io.File;
import java.text.DateFormat;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;

public final class MainActivity extends Activity {
    private TextView statusView;
    private TextView logView;
    private File logDir;

    static {
        // 宿主 App 需要先加载自己的 shared library，JNI 符号和静态库代码都会被打进这个 so。
        System.loadLibrary("zwcadguarddemo");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setupUI();

        logDir = new File(getFilesDir(), "zwcadguard_logs");
        ZWCADGuard.initialize(getApplicationContext(), logDir.getAbsolutePath());
        ZWCADGuard.setActiveDrawingContext(
                "building_structure_layout.dwg",
                "/storage/emulated/0/ZWCAD/Projects/BuildingDesign/building_structure_layout.dwg",
                4598102L,
                "2a6b98e1f0e2d3c456897bc8a9f0e1d2",
                "dwg_file_1024",
                "project_365_a01",
                "BuildingDesign");
        ZWCADGuard.addBreadcrumb("App", "Launch", "Android demo created");
        statusView.setText("ZWCADGuard Android demo initialized.\nLog dir: " + logDir.getAbsolutePath());
        refreshLogSummary();
    }

    private void setupUI() {
        ScrollView scrollView = new ScrollView(this);
        LinearLayout container = new LinearLayout(this);
        container.setOrientation(LinearLayout.VERTICAL);
        int padding = dp(16);
        container.setPadding(padding, padding, padding, padding);

        statusView = new TextView(this);
        statusView.setTextSize(14f);
        container.addView(statusView);

        container.addView(createButton("记录缩放面包屑", v -> {
            ZWCADGuard.addBreadcrumb("Gesture", "Zoom", "scale=1.25");
            statusView.setText("Breadcrumb recorded: Gesture / Zoom");
            refreshLogSummary();
        }));

        container.addView(createButton("触发受控 JNI C++ 异常", v -> {
            try {
                NativeBridge.callGuardedCppException();
            } catch (RuntimeException e) {
                statusView.setText("Guarded JNI exception converted to Java RuntimeException:\n" + e.getMessage());
                refreshLogSummary();
            }
        }));

        container.addView(createButton("刷新本地崩溃日志", v -> refreshLogSummary()));

        container.addView(createButton("触发未捕获 Java 异常", v -> {
            ZWCADGuard.addBreadcrumb("Test", "JavaCrash", "IllegalStateException");
            throw new IllegalStateException("Java crash from demo");
        }));

        container.addView(createButton("触发未捕获 C++ 异常", v -> {
            ZWCADGuard.addBreadcrumb("Test", "CppCrash", "Unhandled std::runtime_error");
            NativeBridge.triggerUnhandledCppException();
        }));

        container.addView(createButton("触发 native signal 崩溃", v -> {
            ZWCADGuard.addBreadcrumb("Test", "SignalCrash", "SIGSEGV");
            NativeBridge.triggerSignalCrash();
        }));

        logView = new TextView(this);
        logView.setTextSize(13f);
        int logPaddingTop = dp(12);
        logView.setPadding(0, logPaddingTop, 0, 0);
        container.addView(logView);

        scrollView.addView(container);
        setContentView(scrollView);
    }

    private void refreshLogSummary() {
        if (logView == null || logDir == null) {
            return;
        }

        File[] files = logDir.listFiles();
        if (files == null || files.length == 0) {
            logView.setText("本地崩溃日志: 暂无");
            return;
        }

        Arrays.sort(files, Comparator.comparingLong(File::lastModified).reversed());
        StringBuilder builder = new StringBuilder();
        builder.append("本地崩溃日志:\n");
        int limit = Math.min(files.length, 8);
        DateFormat format = android.text.format.DateFormat.getMediumDateFormat(this);
        for (int i = 0; i < limit; i++) {
            File file = files[i];
            builder.append(i + 1)
                    .append(". ")
                    .append(file.getName())
                    .append(" (")
                    .append(file.length())
                    .append(" bytes, ")
                    .append(format.format(new Date(file.lastModified())))
                    .append(")\n");
        }
        if (files.length > limit) {
            builder.append("...");
        }
        logView.setText(builder.toString().trim());
    }

    private Button createButton(String title, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(title);
        button.setOnClickListener(listener);
        return button;
    }

    private int dp(int value) {
        float density = getResources().getDisplayMetrics().density;
        return (int) (value * density);
    }
}
