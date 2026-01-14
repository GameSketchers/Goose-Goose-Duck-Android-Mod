package com.android.support;

import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

public class ObbInstallerActivity extends Activity {

    //================== THEME ==================//
    private static final int BG_PRIMARY = Color.parseColor("#FF16141F");
    private static final int BG_SECONDARY = Color.parseColor("#FF1E1B2E");
    private static final int BG_CARD = Color.parseColor("#FF252236");
    private static final int ACCENT_PINK = Color.parseColor("#FFD4A5C9");
    private static final int ACCENT_PURPLE = Color.parseColor("#FFA99FD3");
    private static final int TEXT_PRIMARY = Color.parseColor("#FFF5F5F7");
    private static final int TEXT_SECONDARY = Color.parseColor("#FFB8B5C8");
    private static final int TEXT_MUTED = Color.parseColor("#FF6E6A80");
    private static final int STATE_ON = Color.parseColor("#FF7EC9A0");
    private static final int STATE_OFF = Color.parseColor("#FFE08080");
    private static final int BORDER_ACCENT = Color.parseColor("#44D4A5C9");

    //================== OBB ==================//
    private static final String OBB_FILE_NAME = "main.200524.com.Gaggle.fun.GooseGooseDuck.obb";

    //================== UI ==================//
    private TextView statusText;
    private TextView percentText;
    private View progressFill;
    private FrameLayout progressFrame;
    private Handler mainHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            getWindow().setStatusBarColor(BG_PRIMARY);
            getWindow().setNavigationBarColor(BG_PRIMARY);
        }

        mainHandler = new Handler(Looper.getMainLooper());
        setContentView(createUI());

        // Kurulumu başlat
        mainHandler.postDelayed(new Runnable() {
				@Override
				public void run() {
					startInstallation();
				}
			}, 300);
    }

    private View createUI() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setBackgroundColor(BG_PRIMARY);
        root.setPadding(dp(32), dp(32), dp(32), dp(32));

        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER_HORIZONTAL);
        LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(dp(300), LinearLayout.LayoutParams.WRAP_CONTENT);
        card.setLayoutParams(cardParams);
        card.setPadding(dp(24), dp(28), dp(24), dp(24));

        GradientDrawable cardBg = new GradientDrawable();
        cardBg.setCornerRadius(dp(24));
        cardBg.setColor(BG_CARD);
        cardBg.setStroke(dp(2), BORDER_ACCENT);
        card.setBackground(cardBg);

        // Icon
        FrameLayout iconFrame = new FrameLayout(this);
        LinearLayout.LayoutParams iconFrameParams = new LinearLayout.LayoutParams(dp(80), dp(80));
        iconFrameParams.gravity = Gravity.CENTER;
        iconFrame.setLayoutParams(iconFrameParams);

        View iconBg = new View(this);
        iconBg.setLayoutParams(new FrameLayout.LayoutParams(dp(80), dp(80)));
        GradientDrawable iconDrawable = new GradientDrawable();
        iconDrawable.setShape(GradientDrawable.OVAL);
        iconDrawable.setColor(BG_SECONDARY);
        iconDrawable.setStroke(dp(3), ACCENT_PINK);
        iconBg.setBackground(iconDrawable);

        TextView iconText = new TextView(this);
        FrameLayout.LayoutParams iconTextParams = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT,
            FrameLayout.LayoutParams.WRAP_CONTENT
        );
        iconTextParams.gravity = Gravity.CENTER;
        iconText.setLayoutParams(iconTextParams);
        iconText.setText("🦆");
        iconText.setTextSize(38f);

        iconFrame.addView(iconBg);
        iconFrame.addView(iconText);
        card.addView(iconFrame);

        // Title
        TextView title = new TextView(this);
        LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        titleParams.topMargin = dp(16);
        title.setLayoutParams(titleParams);
        title.setText("Goose Goose Duck");
        title.setTextColor(TEXT_PRIMARY);
        title.setTextSize(20f);
        title.setTypeface(Typeface.create("sans-serif-medium", Typeface.BOLD));
        card.addView(title);

        // Subtitle
        TextView subtitle = new TextView(this);
        LinearLayout.LayoutParams subParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        subParams.topMargin = dp(4);
        subtitle.setLayoutParams(subParams);
        subtitle.setText("Installing Game Data");
        subtitle.setTextColor(ACCENT_PURPLE);
        subtitle.setTextSize(12f);
        card.addView(subtitle);

        // Divider
        View divider = new View(this);
        LinearLayout.LayoutParams divParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, dp(1)
        );
        divParams.topMargin = dp(20);
        divider.setLayoutParams(divParams);
        divider.setBackgroundColor(Color.parseColor("#22FFFFFF"));
        card.addView(divider);

        // Status
        statusText = new TextView(this);
        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        statusParams.topMargin = dp(20);
        statusText.setLayoutParams(statusParams);
        statusText.setText("Preparing...");
        statusText.setTextColor(TEXT_SECONDARY);
        statusText.setTextSize(13f);
        statusText.setGravity(Gravity.CENTER);
        card.addView(statusText);

        // Progress Frame
        progressFrame = new FrameLayout(this);
        LinearLayout.LayoutParams pfParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, dp(12)
        );
        pfParams.topMargin = dp(20);
        progressFrame.setLayoutParams(pfParams);

        View progressBg = new View(this);
        progressBg.setLayoutParams(new FrameLayout.LayoutParams(
									   FrameLayout.LayoutParams.MATCH_PARENT,
									   FrameLayout.LayoutParams.MATCH_PARENT
								   ));
        GradientDrawable bgDrawable = new GradientDrawable();
        bgDrawable.setCornerRadius(dp(6));
        bgDrawable.setColor(BG_SECONDARY);
        progressBg.setBackground(bgDrawable);

        progressFill = new View(this);
        FrameLayout.LayoutParams fillParams = new FrameLayout.LayoutParams(0, FrameLayout.LayoutParams.MATCH_PARENT);
        progressFill.setLayoutParams(fillParams);
        GradientDrawable fillDrawable = new GradientDrawable();
        fillDrawable.setCornerRadius(dp(6));
        fillDrawable.setColor(ACCENT_PINK);
        progressFill.setBackground(fillDrawable);

        progressFrame.addView(progressBg);
        progressFrame.addView(progressFill);
        card.addView(progressFrame);

        // Percent
        percentText = new TextView(this);
        LinearLayout.LayoutParams percentParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        percentParams.topMargin = dp(12);
        percentText.setLayoutParams(percentParams);
        percentText.setText("0%");
        percentText.setTextColor(ACCENT_PINK);
        percentText.setTextSize(24f);
        percentText.setTypeface(Typeface.DEFAULT_BOLD);
        percentText.setGravity(Gravity.CENTER);
        card.addView(percentText);

        // Footer
        TextView footer = new TextView(this);
        LinearLayout.LayoutParams footerParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        footerParams.topMargin = dp(16);
        footer.setLayoutParams(footerParams);
        footer.setText("Please wait...");
        footer.setTextColor(TEXT_MUTED);
        footer.setTextSize(11f);
        footer.setGravity(Gravity.CENTER);
        card.addView(footer);

        root.addView(card);

        // Bottom footer
        TextView bottomFooter = new TextView(this);
        LinearLayout.LayoutParams bfParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        bfParams.topMargin = dp(24);
        bottomFooter.setLayoutParams(bfParams);
        bottomFooter.setText("by anonimbiri");
        bottomFooter.setTextColor(TEXT_MUTED);
        bottomFooter.setTextSize(11f);
        root.addView(bottomFooter);

        return root;
    }

    private void startInstallation() {
        new Thread(new Runnable() {
				@Override
				public void run() {
					boolean success = copyObbFile();
					final boolean finalSuccess = success;

					mainHandler.post(new Runnable() {
							@Override
							public void run() {
								onInstallComplete(finalSuccess);
							}
						});
				}
			}).start();
    }

    private boolean copyObbFile() {
        InputStream is = null;
        OutputStream os = null;

        try {
            File obbDir = getObbDir();
            if (!obbDir.exists()) obbDir.mkdirs();

            File targetFile = new File(obbDir, OBB_FILE_NAME);

            // Boyut al
            long totalSize = 0;
            try {
                AssetFileDescriptor afd = getAssets().openFd(OBB_FILE_NAME);
                totalSize = afd.getLength();
                afd.close();
            } catch (Exception e) {
                // Compressed olabilir
            }

            // Zaten varsa ve boyut doğruysa atla
            if (targetFile.exists() && totalSize > 0 && targetFile.length() == totalSize) {
                updateUI(100, "Already installed!");
                return true;
            }

            if (targetFile.exists()) targetFile.delete();

            updateUI(0, "Opening file...");

            is = getAssets().open(OBB_FILE_NAME);
            os = new FileOutputStream(targetFile);

            if (totalSize == 0) {
                totalSize = is.available();
            }

            byte[] buffer = new byte[16384];
            long copied = 0;
            int length;
            int lastPercent = -1;

            while ((length = is.read(buffer)) > 0) {
                os.write(buffer, 0, length);
                copied += length;

                int percent = totalSize > 0 ? (int) ((copied * 100) / totalSize) : 0;
                if (percent != lastPercent) {
                    lastPercent = percent;
                    final String status = "Installing... " + formatSize(copied) + " / " + formatSize(totalSize);
                    updateUI(percent, status);
                }
            }

            os.flush();
            updateUI(100, "Complete!");
            return true;

        } catch (Exception e) {
            e.printStackTrace();
            updateUI(-1, "Error: " + e.getMessage());
            return false;
        } finally {
            try {
                if (is != null) is.close();
                if (os != null) os.close();
            } catch (Exception e) {}
        }
    }

    private void updateUI(final int percent, final String status) {
        mainHandler.post(new Runnable() {
				@Override
				public void run() {
					if (percent >= 0) {
						percentText.setText(percent + "%");
						updateProgressBar(percent);
					}
					if (status != null) {
						statusText.setText(status);
					}
				}
			});
    }

    private void updateProgressBar(final int percent) {
        progressFrame.post(new Runnable() {
				@Override
				public void run() {
					int parentWidth = progressFrame.getWidth();
					if (parentWidth <= 0) return;

					int targetWidth = (int) (parentWidth * (percent / 100.0f));
					FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) progressFill.getLayoutParams();
					params.width = targetWidth;
					progressFill.setLayoutParams(params);
				}
			});
    }

    private void onInstallComplete(boolean success) {
        if (success) {
            statusText.setText("Installation complete!");
            statusText.setTextColor(STATE_ON);
            percentText.setText("✓");
            percentText.setTextColor(STATE_ON);

            GradientDrawable fillDrawable = new GradientDrawable();
            fillDrawable.setCornerRadius(dp(6));
            fillDrawable.setColor(STATE_ON);
            progressFill.setBackground(fillDrawable);

            // 1 saniye bekle, Main.Start çağır ve kapat
            mainHandler.postDelayed(new Runnable() {
					@Override
					public void run() {
						// Kurulum tamam - izin akışına devam et
						Main.Start(ObbInstallerActivity.this);
						finish();
					}
				}, 1000);
        } else {
            statusText.setText("Installation failed!");
            statusText.setTextColor(STATE_OFF);
            percentText.setText("✗");
            percentText.setTextColor(STATE_OFF);

            GradientDrawable fillDrawable = new GradientDrawable();
            fillDrawable.setCornerRadius(dp(6));
            fillDrawable.setColor(STATE_OFF);
            progressFill.setBackground(fillDrawable);

            // 2 saniye bekle ve kapat
            mainHandler.postDelayed(new Runnable() {
					@Override
					public void run() {
						finish();
					}
				}, 2000);
        }
    }

    private String formatSize(long bytes) {
        if (bytes >= 1024L * 1024L * 1024L) {
            return String.format("%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
        } else if (bytes >= 1024L * 1024L) {
            return String.format("%.1f MB", bytes / (1024.0 * 1024.0));
        } else if (bytes >= 1024L) {
            return String.format("%.1f KB", bytes / 1024.0);
        }
        return bytes + " B";
    }

    private int dp(int value) {
        return (int) TypedValue.applyDimension(
            TypedValue.COMPLEX_UNIT_DIP, value,
            getResources().getDisplayMetrics()
        );
    }

    @Override
    public void onBackPressed() {
        // Block
    }
}
