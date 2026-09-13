package com.android.support;

import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.text.Html;
import android.text.InputType;
import android.text.TextUtils;
import android.util.Base64;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.OvershootInterpolator;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

public class Menu {

    public static final String TAG = "Mod_Menu";

    private static ESPView espView;
    private static WindowManager espWindowManager;
    private static Handler espHandler;
    private static boolean espEnabled = false;
    private static Menu instance;

    private Typeface menuFont;
    private Typeface iconFont;
    private boolean fontsLoaded = false;

    int BG_PRIMARY = Color.parseColor("#FF16141F");
    int BG_SECONDARY = Color.parseColor("#FF1E1B2E");
    int BG_CARD = Color.parseColor("#FF252236");
    int BG_CARD_LIGHT = Color.parseColor("#FF2E2A42");
    int BG_INPUT = Color.parseColor("#FF3A3552");

    int ACCENT_PINK = Color.parseColor("#FFD4A5C9");
    int ACCENT_PURPLE = Color.parseColor("#FFA99FD3");
    int ACCENT_BLUE = Color.parseColor("#FF8EC8E8");
    int ACCENT_MINT = Color.parseColor("#FF98D9C2");
    int ACCENT_PEACH = Color.parseColor("#FFFFC4A3");
    int ACCENT_ROSE = Color.parseColor("#FFE8A0A0");

    int TEXT_PRIMARY = Color.parseColor("#FFF5F5F7");
    int TEXT_SECONDARY = Color.parseColor("#FFB8B5C8");
    int TEXT_MUTED = Color.parseColor("#FF6E6A80");

    int STATE_ON = Color.parseColor("#FF7EC9A0");
    int STATE_OFF = Color.parseColor("#FFE08080");

    int BORDER_SOFT = Color.parseColor("#33FFFFFF");
    int BORDER_ACCENT = Color.parseColor("#44D4A5C9");

    int MENU_WIDTH = 305;
    int MENU_HEIGHT = 205;
    int PADDING = 16;
    int ITEM_SPACING = 8;
    int RADIUS_XL = 24;
    int RADIUS_L = 16;
    int RADIUS_M = 12;
    int RADIUS_S = 8;
    int ICON_SIZE = 50;
    float ICON_ALPHA = 0.9f;

    int POS_X = 0;
    int POS_Y = 100;

    RelativeLayout mRootContainer;
    FrameLayout mCollapsed;
    LinearLayout mExpanded, mods, mSettings, mCollapse;
    LinearLayout.LayoutParams scrlLLExpanded, scrlLL;
    WindowManager mWindowManager;
    WindowManager.LayoutParams vmParams;
    FrameLayout rootFrame;
    ScrollView scrollView;
    boolean stopChecking, overlayRequired;
    Context getContext;

    native void Init(Context context, TextView title, TextView subTitle);
    native String Icon();
    native String IconWebViewData();
    native String[] GetFeatureList();
    native String[] SettingsList();
    native boolean IsGameLibLoaded();

    public Menu(Context context) {
        getContext = context;
        instance = this;
        Preferences.context = context;

        loadFonts(context);

        rootFrame = new FrameLayout(context);
        mRootContainer = new RelativeLayout(context);

        mCollapsed = new FrameLayout(context);
        mCollapsed.setLayoutParams(new RelativeLayout.LayoutParams(dp(ICON_SIZE), dp(ICON_SIZE)));
        mCollapsed.setVisibility(View.VISIBLE);
        mCollapsed.setAlpha(ICON_ALPHA);
        setupIcon(context);

        mExpanded = new LinearLayout(context);
        mExpanded.setVisibility(View.GONE);
        mExpanded.setOrientation(LinearLayout.VERTICAL);
        mExpanded.setLayoutParams(new LinearLayout.LayoutParams(dp(MENU_WIDTH), WRAP_CONTENT));
        setupMenuStyle();

        mSettings = new LinearLayout(context);
        mSettings.setOrientation(LinearLayout.VERTICAL);
        mSettings.setPadding(dp(PADDING), dp(10), dp(PADDING), dp(10));

        LinearLayout header = buildHeader(context);
        TextView subtitle = buildSubtitle(context);
        View divTop = buildDivider(context);

        scrollView = new ScrollView(context);
        scrlLL = new LinearLayout.LayoutParams(MATCH_PARENT, dp(MENU_HEIGHT));
        scrlLLExpanded = new LinearLayout.LayoutParams(MATCH_PARENT, 0, 1f);
        scrollView.setLayoutParams(Preferences.isExpanded ? scrlLLExpanded : scrlLL);
        scrollView.setVerticalScrollBarEnabled(false);
        scrollView.setOverScrollMode(View.OVER_SCROLL_NEVER);

        mods = new LinearLayout(context);
        mods.setOrientation(LinearLayout.VERTICAL);
        mods.setPadding(dp(PADDING), dp(10), dp(PADDING), dp(10));

        View divBottom = buildDivider(context);
        LinearLayout footer = buildFooter(context);

        mRootContainer.addView(mCollapsed);
        mRootContainer.addView(mExpanded);

        mExpanded.addView(header);
        mExpanded.addView(subtitle);
        mExpanded.addView(divTop);
        scrollView.addView(mods);
        mExpanded.addView(scrollView);
        mExpanded.addView(divBottom);
        mExpanded.addView(footer);

        featureList(SettingsList(), mSettings);
        Init(context, (TextView) header.getChildAt(0), subtitle);
    }

    private void loadFonts(Context context) {
        try {
            menuFont = Typeface.createFromAsset(context.getAssets(), "fonts/MPLUSRounded1c-Medium.ttf");
            iconFont = Typeface.createFromAsset(context.getAssets(), "fonts/Phosphor-Bold.ttf");
            fontsLoaded = true;
        } catch (Exception e) {
            menuFont = Typeface.create("sans-serif-medium", Typeface.NORMAL);
            iconFont = Typeface.DEFAULT;
            fontsLoaded = false;
        }
    }

    private int getColorForIcon(String icon) {
        if (icon == null || icon.isEmpty()) return ACCENT_PINK;
        char c = icon.charAt(0);
        switch (c) {
            case '\uECB4': case '\uF04B': case '\uF2BF': case '\uEEAF': case '\uEC90': case '\uF447': case '\uEC15': case '\uF403':
                return ACCENT_BLUE;
            case '\uEC11': case '\uED3D': case '\uF093': case '\uEB80': case '\uEB7A': case '\uF00A': case '\uF24F': case '\uEC59':
                return ACCENT_MINT;
            case '\uF103': case '\uEDB4': case '\uF35B': case '\uF17E': case '\uF0EA': case '\uEA08': case '\uEAB2': case '\uEB82': case '\uF04F':
                return ACCENT_PURPLE;
            case '\uF63A': case '\uF3B2': case '\uF1CE': case '\uEB06': case '\uEB96': case '\uEB98': case '\uF147': case '\uF1A0':
                return ACCENT_ROSE;
            case '\uED32': case '\uF185': case '\uEAA5': case '\uF228': case '\uECA0': case '\uECBC': case '\uEA36':
                return ACCENT_PEACH;
            default:
                return ACCENT_PINK;
        }
    }

    public void initESP() {
        if (espView != null) return;
        try {
            espWindowManager = (WindowManager) getContext.getSystemService(Context.WINDOW_SERVICE);
            espView = new ESPView(getContext);
            espView.setIconFont(iconFont);
            espWindowManager.addView(espView, ESPView.createLayoutParams());
            espHandler = new Handler(Looper.getMainLooper());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void batchDrawESP(String data) {
        if (espView == null) return;
        espView.clearAll();
        if (!espEnabled || data == null || data.isEmpty()) {
            espView.refresh();
            return;
        }
        String[] items = data.split(";");
        for (String item : items) {
            if (item.isEmpty() || item.length() < 2) continue;
            try {
                char type = item.charAt(0);
                String content = item.substring(1);
                String[] parts = content.split(",");
                switch (type) {
                    case 'L':
                        if (parts.length >= 5) {
                            espView.addLine(
                                Float.parseFloat(parts[0]),
                                Float.parseFloat(parts[1]),
                                Float.parseFloat(parts[2]),
                                Float.parseFloat(parts[3]),
                                parseColor(parts[4])
                            );
                        }
                        break;
                    case 'B':
                        if (parts.length >= 5) {
                            espView.addBox(
                                Float.parseFloat(parts[0]),
                                Float.parseFloat(parts[1]),
                                Float.parseFloat(parts[2]),
                                Float.parseFloat(parts[3]),
                                parseColor(parts[4])
                            );
                        }
                        break;
                    case 'T':
                        if (parts.length >= 4) {
                            espView.addText(
                                Float.parseFloat(parts[0]),
                                Float.parseFloat(parts[1]),
                                parts[2],
                                parseColor(parts[3])
                            );
                        }
                        break;
                    case 'I':
                        if (parts.length >= 4) {
                            espView.addIcon(
                                Float.parseFloat(parts[0]),
                                Float.parseFloat(parts[1]),
                                parts[2],
                                parseColor(parts[3])
                            );
                        }
                        break;
                    case 'C':
                        if (parts.length >= 4) {
                            espView.addCircle(
                                Float.parseFloat(parts[0]),
                                Float.parseFloat(parts[1]),
                                Float.parseFloat(parts[2]),
                                parseColor(parts[3])
                            );
                        }
                        break;
                    case 'F':
                        if (parts.length >= 5) {
                            espView.addFilledBox(
                                Float.parseFloat(parts[0]),
                                Float.parseFloat(parts[1]),
                                Float.parseFloat(parts[2]),
                                Float.parseFloat(parts[3]),
                                parseColor(parts[4])
                            );
                        }
                        break;
                }
            } catch (Exception e) {}
        }
        espView.refresh();
    }

    private static int parseColor(String colorStr) {
        try {
            return (int) Long.parseLong(colorStr.trim());
        } catch (Exception e) {
            return 0xFFFFFFFF;
        }
    }

    public static void setESPEnabled(boolean enabled) {
        espEnabled = enabled;
        if (espView != null) espView.setDrawing(enabled);
    }

    public static int getScreenWidth() {
        return espView != null ? espView.getRealWidth() : 1080;
    }

    public static int getScreenHeight() {
        return espView != null ? espView.getRealHeight() : 2400;
    }

    public void destroyESP() {
        if (espHandler != null) espHandler.removeCallbacksAndMessages(null);
        if (espView != null && espWindowManager != null) {
            try {
                espWindowManager.removeView(espView);
            } catch (Exception e) {}
            espView = null;
        }
    }

    private TextView createIconView(String iconCode, int color) {
        TextView iconView = new TextView(getContext);
        iconView.setText(iconCode);
        iconView.setTextColor(color);
        iconView.setTextSize(16f);
        iconView.setTypeface(iconFont);
        iconView.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(dp(24), dp(24));
        params.setMargins(0, 0, dp(10), 0);
        iconView.setLayoutParams(params);
        return iconView;
    }

    private void setupIcon(Context context) {
        View ring = new View(context);
        FrameLayout.LayoutParams ringParams = new FrameLayout.LayoutParams(dp(ICON_SIZE), dp(ICON_SIZE));
        ring.setLayoutParams(ringParams);
        GradientDrawable ringBg = new GradientDrawable();
        ringBg.setShape(GradientDrawable.OVAL);
        ringBg.setColor(Color.TRANSPARENT);
        ringBg.setStroke(dp(3), ACCENT_PINK);
        ring.setBackground(ringBg);

        View innerBg = new View(context);
        FrameLayout.LayoutParams innerParams = new FrameLayout.LayoutParams(dp(ICON_SIZE - 8), dp(ICON_SIZE - 8));
        innerParams.gravity = Gravity.CENTER;
        innerBg.setLayoutParams(innerParams);
        GradientDrawable innerDrawable = new GradientDrawable();
        innerDrawable.setShape(GradientDrawable.OVAL);
        innerDrawable.setColor(BG_SECONDARY);
        innerBg.setBackground(innerDrawable);

        mCollapsed.addView(ring);
        mCollapsed.addView(innerBg);

        boolean useWebView = false;
        String webData = null;
        try { webData = IconWebViewData(); } catch (Throwable ignored) {}

        if (webData != null && webData.trim().length() > 10) {
            useWebView = true;
        }

        if (useWebView) {
            WebView wView = new WebView(context);
            FrameLayout.LayoutParams wParams = new FrameLayout.LayoutParams(dp(ICON_SIZE - 14), dp(ICON_SIZE - 14));
            wParams.gravity = Gravity.CENTER;
            wView.setLayoutParams(wParams);
            wView.loadData(
                "<html><body style='margin:0;padding:0;background:transparent;'>" +
                "<img src='" + webData + "' width='100%' height='100%' style='object-fit:contain;'/>" +
                "</body></html>",
                "text/html", "utf-8"
            );
            wView.setBackgroundColor(Color.TRANSPARENT);
            wView.getSettings().setCacheMode(WebSettings.LOAD_NO_CACHE);
            mCollapsed.addView(wView);
        } else {
            ImageView iconImg = new ImageView(context);
            FrameLayout.LayoutParams imgParams = new FrameLayout.LayoutParams(dp(ICON_SIZE - 16), dp(ICON_SIZE - 16));
            imgParams.gravity = Gravity.CENTER;
            iconImg.setLayoutParams(imgParams);
            iconImg.setScaleType(ImageView.ScaleType.FIT_CENTER);
            String base64 = null;
            try { base64 = Icon(); } catch (Throwable ignored) {}
            if (base64 != null && base64.trim().length() > 20) {
                try {
                    byte[] data = Base64.decode(base64, Base64.DEFAULT);
                    iconImg.setImageBitmap(BitmapFactory.decodeByteArray(data, 0, data.length));
                } catch (Exception e) {
                    iconImg.setBackgroundColor(0xFFFF5252);
                }
            } else {
                iconImg.setBackgroundColor(0xFFFF5252);
            }
            mCollapsed.addView(iconImg);
        }
    }

    private void setupMenuStyle() {
        GradientDrawable bg = new GradientDrawable();
        bg.setCornerRadius(dp(RADIUS_XL));
        bg.setColor(BG_PRIMARY);
        bg.setStroke(dp(1), BORDER_ACCENT);
        mExpanded.setBackground(bg);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mExpanded.setElevation(dp(16));
        }
        mExpanded.setClipToOutline(true);
    }

    private LinearLayout buildHeader(Context context) {
        LinearLayout header = new LinearLayout(context);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);
        header.setPadding(dp(PADDING), dp(14), dp(PADDING), dp(8));
        header.setOnTouchListener(menuDragListener());

        TextView title = new TextView(context);
        title.setTextColor(TEXT_PRIMARY);
        title.setTextSize(16f);
        title.setTypeface(menuFont, Typeface.BOLD);
        title.setLayoutParams(new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f));

        final TextView settingsBtn = new TextView(context);
        settingsBtn.setText("\uE270");
        settingsBtn.setTextColor(ACCENT_PURPLE);
        settingsBtn.setTextSize(22f);
        settingsBtn.setTypeface(iconFont);
        settingsBtn.setPadding(dp(12), dp(4), dp(4), dp(4));

        settingsBtn.setOnClickListener(new View.OnClickListener() {
				boolean isSettings = false;
				@Override
				public void onClick(View v) {
					isSettings = !isSettings;
					scrollView.animate().alpha(0f).setDuration(150).withEndAction(new Runnable() {
							@Override
							public void run() {
								scrollView.removeAllViews();
								scrollView.addView(isSettings ? mSettings : mods);
								scrollView.scrollTo(0, 0);
								scrollView.animate().alpha(1f).setDuration(150).start();
							}
						}).start();
                    settingsBtn.setText(isSettings ? "\uE014" : "\uE270");
				}
			});

        header.addView(title);
        header.addView(settingsBtn);
        return header;
    }

    private TextView buildSubtitle(Context context) {
        TextView sub = new TextView(context);
        sub.setEllipsize(TextUtils.TruncateAt.MARQUEE);
        sub.setMarqueeRepeatLimit(-1);
        sub.setSingleLine(true);
        sub.setSelected(true);
        sub.setTextColor(TEXT_MUTED);
        sub.setTextSize(10f);
        sub.setTypeface(menuFont);
        sub.setGravity(Gravity.CENTER);
        sub.setPadding(dp(PADDING), 0, dp(PADDING), dp(10));
        return sub;
    }

    private View buildDivider(Context context) {
        View div = new View(context);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH_PARENT, dp(1));
        params.setMargins(dp(PADDING + 8), 0, dp(PADDING + 8), 0);
        div.setLayoutParams(params);
        div.setBackgroundColor(BORDER_SOFT);
        return div;
    }

    private LinearLayout buildFooter(Context context) {
        LinearLayout footer = new LinearLayout(context);
        footer.setOrientation(LinearLayout.HORIZONTAL);
        footer.setGravity(Gravity.CENTER);
        footer.setPadding(dp(PADDING), dp(12), dp(PADDING), dp(14));
        footer.setWeightSum(2);

        LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(0, dp(40), 1);
        btnParams.setMargins(dp(6), 0, dp(6), 0);

        LinearLayout hideBtn = new LinearLayout(context);
        hideBtn.setLayoutParams(btnParams);
        hideBtn.setGravity(Gravity.CENTER);
        hideBtn.setOrientation(LinearLayout.HORIZONTAL);

        GradientDrawable hideBg = new GradientDrawable();
        hideBg.setCornerRadius(dp(RADIUS_M));
        hideBg.setColor(BG_CARD);
        hideBtn.setBackground(hideBg);

        TextView hideIcon = new TextView(context);
        hideIcon.setText("\uE222");
        hideIcon.setTextColor(TEXT_SECONDARY);
        hideIcon.setTextSize(14f);
        hideIcon.setTypeface(iconFont);

        TextView hideText = new TextView(context);
        hideText.setText("Hide");
        hideText.setTextColor(TEXT_SECONDARY);
        hideText.setTextSize(13f);
        hideText.setTypeface(menuFont);
        hideText.setPadding(dp(6), 0, 0, 0);

        hideBtn.addView(hideIcon);
        hideBtn.addView(hideText);

        hideBtn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					mExpanded.animate().alpha(0f).scaleX(0.9f).scaleY(0.9f)
						.setDuration(200).withEndAction(new Runnable() {
							@Override
							public void run() {
								mExpanded.setVisibility(View.GONE);
								mExpanded.setAlpha(1f);
								mExpanded.setScaleX(1f);
								mExpanded.setScaleY(1f);
								mCollapsed.setVisibility(View.VISIBLE);
								mCollapsed.setAlpha(0f);
							}
						}).start();
					Toast.makeText(getContext, "Icon hidden. Remember position!", Toast.LENGTH_LONG).show();
				}
			});

        hideBtn.setOnLongClickListener(new View.OnLongClickListener() {
				@Override
				public boolean onLongClick(View v) {
					Toast.makeText(getContext, "Menu killed", Toast.LENGTH_SHORT).show();
					if (rootFrame != null && mWindowManager != null) {
						try { mWindowManager.removeView(rootFrame); } catch (Exception e) {}
					}
					return true;
				}
			});

        LinearLayout closeBtn = new LinearLayout(context);
        closeBtn.setLayoutParams(btnParams);
        closeBtn.setGravity(Gravity.CENTER);
        closeBtn.setOrientation(LinearLayout.HORIZONTAL);

        GradientDrawable closeBg = new GradientDrawable();
        closeBg.setCornerRadius(dp(RADIUS_M));
        closeBg.setColor(ACCENT_PINK);
        closeBtn.setBackground(closeBg);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            closeBtn.setElevation(dp(2));
        }

        TextView closeIcon = new TextView(context);
        closeIcon.setText("\uE4F6");
        closeIcon.setTextColor(BG_PRIMARY);
        closeIcon.setTextSize(14f);
        closeIcon.setTypeface(iconFont);

        TextView closeText = new TextView(context);
        closeText.setText("Close");
        closeText.setTextColor(BG_PRIMARY);
        closeText.setTextSize(13f);
        closeText.setTypeface(menuFont, Typeface.BOLD);
        closeText.setPadding(dp(6), 0, 0, 0);

        closeBtn.addView(closeIcon);
        closeBtn.addView(closeText);

        closeBtn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					mExpanded.animate().alpha(0f).scaleX(0.9f).scaleY(0.9f)
						.setDuration(200).setInterpolator(new DecelerateInterpolator())
						.withEndAction(new Runnable() {
							@Override
							public void run() {
								mExpanded.setVisibility(View.GONE);
								mExpanded.setAlpha(1f);
								mExpanded.setScaleX(1f);
								mExpanded.setScaleY(1f);
								mCollapsed.setVisibility(View.VISIBLE);
								mCollapsed.setAlpha(ICON_ALPHA);
							}
						}).start();
				}
			});

        footer.addView(hideBtn);
        footer.addView(closeBtn);
        return footer;
    }

    private LinearLayout makeCard(boolean vertical) {
        LinearLayout card = new LinearLayout(getContext);
        card.setOrientation(vertical ? LinearLayout.VERTICAL : LinearLayout.HORIZONTAL);
        if (!vertical) card.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT);
        params.setMargins(0, dp(ITEM_SPACING), 0, dp(ITEM_SPACING));
        card.setLayoutParams(params);
        card.setPadding(dp(14), dp(14), dp(14), dp(14));
        GradientDrawable bg = new GradientDrawable();
        bg.setCornerRadius(dp(RADIUS_L));
        bg.setColor(BG_CARD);
        card.setBackground(bg);
        return card;
    }

    private String extractIcon(String feature) {
        if (feature.contains("[") && feature.contains("]")) {
            int start = feature.indexOf("[");
            int end = feature.indexOf("]");
            if (start < end && start >= 0) {
                return feature.substring(start + 1, end);
            }
        }
        return null;
    }

    private String removeIconFromName(String name) {
        if (name.contains("[") && name.contains("]")) {
            int start = name.indexOf("[");
            int end = name.indexOf("]");
            if (start < end && start >= 0) {
                return name.substring(0, start) + name.substring(end + 1);
            }
        }
        return name;
    }

    private void Switch(LinearLayout parent, final int featNum, final String featName, boolean defaultOn, String icon, int iconColor) {
        LinearLayout card = makeCard(false);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(iconColor);
            iconView.setTextSize(18f);
            iconView.setTypeface(iconFont);
            LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(dp(24), dp(24));
            iconParams.setMargins(0, 0, dp(10), 0);
            iconView.setLayoutParams(iconParams);
            iconView.setGravity(Gravity.CENTER);
            card.addView(iconView);
        }

        TextView label = new TextView(getContext);
        label.setText(featName);
        label.setTextColor(TEXT_PRIMARY);
        label.setTextSize(14f);
        label.setTypeface(menuFont);
        label.setLayoutParams(new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f));

        final FrameLayout switchFrame = new FrameLayout(getContext);
        switchFrame.setLayoutParams(new LinearLayout.LayoutParams(dp(48), dp(26)));

        final View track = new View(getContext);
        track.setLayoutParams(new FrameLayout.LayoutParams(dp(48), dp(26)));

        final View thumb = new View(getContext);
        FrameLayout.LayoutParams thumbParams = new FrameLayout.LayoutParams(dp(20), dp(20));
        thumbParams.gravity = Gravity.CENTER_VERTICAL;
        thumb.setLayoutParams(thumbParams);

        switchFrame.addView(track);
        switchFrame.addView(thumb);

        final boolean[] isOn = {Preferences.loadPrefBool(featName, featNum, defaultOn)};
        applySwitchStyle(track, thumb, isOn[0], false);

        View.OnClickListener toggle = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                isOn[0] = !isOn[0];
                applySwitchStyle(track, thumb, isOn[0], true);
                Preferences.changeFeatureBool(featName, featNum, isOn[0]);
                if (featNum == -1) {
                    Preferences.with(getContext).writeBoolean(-1, isOn[0]);
                    if (!isOn[0]) Preferences.with(getContext).clear();
                } else if (featNum == -3) {
                    Preferences.isExpanded = isOn[0];
                    scrollView.setLayoutParams(isOn[0] ? scrlLLExpanded : scrlLL);
                }
            }
        };

        card.setOnClickListener(toggle);
        card.addView(label);
        card.addView(switchFrame);
        parent.addView(card);
    }

    private void applySwitchStyle(View track, final View thumb, boolean on, boolean animate) {
        GradientDrawable trackBg = new GradientDrawable();
        trackBg.setCornerRadius(dp(13));
        trackBg.setColor(on ? ACCENT_PINK : BG_INPUT);
        track.setBackground(trackBg);

        GradientDrawable thumbBg = new GradientDrawable();
        thumbBg.setShape(GradientDrawable.OVAL);
        thumbBg.setColor(on ? TEXT_PRIMARY : TEXT_MUTED);
        thumb.setBackground(thumbBg);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            thumb.setElevation(dp(2));
        }

        final int targetMargin = on ? dp(25) : dp(3);

        if (animate) {
            FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) thumb.getLayoutParams();
            int startMargin = params.leftMargin;
            ValueAnimator anim = ValueAnimator.ofInt(startMargin, targetMargin);
            anim.setDuration(200);
            anim.setInterpolator(new OvershootInterpolator(1.5f));
            anim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
					@Override
					public void onAnimationUpdate(ValueAnimator animation) {
						FrameLayout.LayoutParams p = (FrameLayout.LayoutParams) thumb.getLayoutParams();
						p.leftMargin = (int) animation.getAnimatedValue();
						thumb.setLayoutParams(p);
					}
				});
            anim.start();
        } else {
            FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) thumb.getLayoutParams();
            params.leftMargin = targetMargin;
            thumb.setLayoutParams(params);
        }
    }

    private void SeekBar(LinearLayout parent, final int featNum, final String featName, final int min, final int max, String icon, int iconColor) {
        int current = Preferences.loadPrefInt(featName, featNum);
        if (current < min) current = min;

        LinearLayout card = makeCard(true);

        LinearLayout header = new LinearLayout(getContext);
        header.setGravity(Gravity.CENTER_VERTICAL);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(iconColor);
            iconView.setTextSize(16f);
            iconView.setTypeface(iconFont);
            LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(dp(22), dp(22));
            iconParams.setMargins(0, 0, dp(8), 0);
            iconView.setLayoutParams(iconParams);
            iconView.setGravity(Gravity.CENTER);
            header.addView(iconView);
        }

        TextView label = new TextView(getContext);
        label.setText(featName);
        label.setTextColor(TEXT_PRIMARY);
        label.setTextSize(14f);
        label.setTypeface(menuFont);
        label.setLayoutParams(new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f));

        final int sliderColor = (icon != null) ? iconColor : ACCENT_BLUE;

        final TextView value = new TextView(getContext);
        value.setText(String.valueOf(current));
        value.setTextColor(sliderColor);
        value.setTextSize(14f);
        value.setTypeface(menuFont, Typeface.BOLD);

        header.addView(label);
        header.addView(value);

        final FrameLayout sliderFrame = new FrameLayout(getContext);
        LinearLayout.LayoutParams sfParams = new LinearLayout.LayoutParams(MATCH_PARENT, dp(36));
        sfParams.topMargin = dp(12);
        sliderFrame.setLayoutParams(sfParams);
        sliderFrame.setPadding(dp(10), 0, dp(10), 0);

        View bgTrack = new View(getContext);
        FrameLayout.LayoutParams bgParams = new FrameLayout.LayoutParams(MATCH_PARENT, dp(6));
        bgParams.gravity = Gravity.CENTER_VERTICAL;
        bgTrack.setLayoutParams(bgParams);
        GradientDrawable bgBg = new GradientDrawable();
        bgBg.setCornerRadius(dp(3));
        bgBg.setColor(BG_INPUT);
        bgTrack.setBackground(bgBg);

        final View progressTrack = new View(getContext);
        FrameLayout.LayoutParams progParams = new FrameLayout.LayoutParams(dp(6), dp(6));
        progParams.gravity = Gravity.CENTER_VERTICAL;
        progressTrack.setLayoutParams(progParams);
        GradientDrawable progBg = new GradientDrawable();
        progBg.setCornerRadius(dp(3));
        progBg.setColor(sliderColor);
        progressTrack.setBackground(progBg);

        final android.widget.SeekBar seekBar = new android.widget.SeekBar(getContext);
        FrameLayout.LayoutParams sbParams = new FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT);
        seekBar.setLayoutParams(sbParams);
        seekBar.setMax(max - min);
        seekBar.setProgress(current - min);
        seekBar.setPadding(0, 0, 0, 0);
        seekBar.setBackground(null);
        seekBar.getProgressDrawable().setAlpha(0);
        seekBar.setSplitTrack(false);

        GradientDrawable thumbD = new GradientDrawable();
        thumbD.setShape(GradientDrawable.OVAL);
        thumbD.setSize(dp(20), dp(20));
        thumbD.setColor(TEXT_PRIMARY);
        thumbD.setStroke(dp(3), sliderColor);
        seekBar.setThumb(thumbD);
        seekBar.setThumbOffset(0);

        seekBar.setOnSeekBarChangeListener(new android.widget.SeekBar.OnSeekBarChangeListener() {
				@Override
				public void onProgressChanged(android.widget.SeekBar sb, int prog, boolean user) {
					int val = prog + min;
					value.setText(String.valueOf(val));
					Preferences.changeFeatureInt(featName, featNum, val);
					float pct = (max > min) ? (float) prog / (max - min) : 0;
					int trackWidth = (int) ((sliderFrame.getWidth() - dp(20)) * pct) + dp(6);
					FrameLayout.LayoutParams pp = (FrameLayout.LayoutParams) progressTrack.getLayoutParams();
					pp.width = Math.max(dp(6), trackWidth);
					progressTrack.setLayoutParams(pp);
				}
				@Override public void onStartTrackingTouch(android.widget.SeekBar sb) {}
				@Override public void onStopTrackingTouch(android.widget.SeekBar sb) {}
			});

        sliderFrame.post(new Runnable() {
				@Override
				public void run() {
					int prog = seekBar.getProgress();
					float pct = (max > min) ? (float) prog / (max - min) : 0;
					int trackWidth = (int) ((sliderFrame.getWidth() - dp(20)) * pct) + dp(6);
					FrameLayout.LayoutParams pp = (FrameLayout.LayoutParams) progressTrack.getLayoutParams();
					pp.width = Math.max(dp(6), trackWidth);
					progressTrack.setLayoutParams(pp);
				}
			});

        sliderFrame.addView(bgTrack);
        sliderFrame.addView(progressTrack);
        sliderFrame.addView(seekBar);

        card.addView(header);
        card.addView(sliderFrame);
        parent.addView(card);
    }

    private void Button(LinearLayout parent, final int featNum, final String featName, String icon, int iconColor) {
        LinearLayout btn = new LinearLayout(getContext);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH_PARENT, dp(48));
        params.setMargins(0, dp(ITEM_SPACING), 0, dp(ITEM_SPACING));
        btn.setLayoutParams(params);
        btn.setGravity(Gravity.CENTER);
        btn.setOrientation(LinearLayout.HORIZONTAL);

        int bgColor = (icon != null) ? iconColor : ACCENT_PINK;

        GradientDrawable bg = new GradientDrawable();
        bg.setCornerRadius(dp(RADIUS_L));
        bg.setColor(bgColor);
        btn.setBackground(bg);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            btn.setElevation(dp(2));
        }

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(BG_PRIMARY);
            iconView.setTextSize(16f);
            iconView.setTypeface(iconFont);
            iconView.setPadding(0, 0, dp(8), 0);
            btn.addView(iconView);
        }

        TextView text = new TextView(getContext);
        text.setText(Html.fromHtml(featName));
        text.setTextColor(BG_PRIMARY);
        text.setTextSize(14f);
        text.setTypeface(menuFont, Typeface.BOLD);
        btn.addView(text);

        btn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(final View v) {
					v.animate().scaleX(0.95f).scaleY(0.95f).setDuration(80)
						.withEndAction(new Runnable() {
							@Override public void run() {
								v.animate().scaleX(1f).scaleY(1f).setDuration(80).start();
							}
						}).start();
					if (featNum == -6) {
						scrollView.removeAllViews();
						scrollView.addView(mods);
					} else if (featNum == -100) {
						stopChecking = true;
					}
					Preferences.changeFeatureInt(featName, featNum, 0);
				}
			});

        parent.addView(btn);
    }

    private void ButtonOnOff(LinearLayout parent, final int featNum, String featName, boolean defaultOn, final String icon) {
        final String name = featName.replace("OnOff_", "");
        final boolean[] isOn = {Preferences.loadPrefBool(featName, featNum, defaultOn)};

        final LinearLayout btn = new LinearLayout(getContext);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH_PARENT, dp(48));
        params.setMargins(0, dp(ITEM_SPACING), 0, dp(ITEM_SPACING));
        btn.setLayoutParams(params);
        btn.setGravity(Gravity.CENTER);
        btn.setOrientation(LinearLayout.HORIZONTAL);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            btn.setElevation(dp(2));
        }

        final TextView iconView;
        if (icon != null) {
            iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextSize(16f);
            iconView.setTypeface(iconFont);
            iconView.setPadding(0, 0, dp(8), 0);
            btn.addView(iconView);
        } else {
            iconView = null;
        }

        final TextView text = new TextView(getContext);
        text.setTextSize(14f);
        text.setTypeface(menuFont, Typeface.BOLD);
        btn.addView(text);

        applyOnOffStyle(btn, iconView, text, name, isOn[0]);

        btn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(final View v) {
					isOn[0] = !isOn[0];
					applyOnOffStyle(btn, iconView, text, name, isOn[0]);
					Preferences.changeFeatureBool(name, featNum, isOn[0]);
					v.animate().scaleX(0.95f).scaleY(0.95f).setDuration(80)
						.withEndAction(new Runnable() {
							@Override public void run() {
								v.animate().scaleX(1f).scaleY(1f).setDuration(80).start();
							}
						}).start();
				}
			});

        parent.addView(btn);
    }

    private void applyOnOffStyle(LinearLayout btn, TextView iconView, TextView text, String name, boolean on) {
        text.setText(name + (on ? "  :  ON" : "  :  OFF"));
        text.setTextColor(TEXT_PRIMARY);
        if (iconView != null) iconView.setTextColor(on ? STATE_ON : STATE_OFF);
        GradientDrawable bg = new GradientDrawable();
        bg.setCornerRadius(dp(RADIUS_L));
        bg.setColor(on ? STATE_ON : STATE_OFF);
        btn.setBackground(bg);
    }

    private void ButtonLink(LinearLayout parent, final String name, final String url, String icon, int accentColor) {
        LinearLayout btn = new LinearLayout(getContext);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH_PARENT, dp(44));
        params.setMargins(0, dp(ITEM_SPACING), 0, dp(ITEM_SPACING));
        btn.setLayoutParams(params);
        btn.setGravity(Gravity.CENTER);
        btn.setOrientation(LinearLayout.HORIZONTAL);

        int linkColor = (icon != null) ? accentColor : ACCENT_BLUE;

        GradientDrawable bg = new GradientDrawable();
        bg.setCornerRadius(dp(RADIUS_L));
        bg.setStroke(dp(1), linkColor);
        btn.setBackground(bg);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(linkColor);
            iconView.setTextSize(14f);
            iconView.setTypeface(iconFont);
            iconView.setPadding(0, 0, dp(8), 0);
            btn.addView(iconView);
        }

        TextView text = new TextView(getContext);
        text.setText(Html.fromHtml(name));
        text.setTextColor(linkColor);
        text.setTextSize(14f);
        text.setTypeface(menuFont);
        btn.addView(text);

        btn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					Intent i = new Intent(Intent.ACTION_VIEW);
					i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					i.setData(Uri.parse(url));
					getContext.startActivity(i);
				}
			});

        parent.addView(btn);
    }

    private void Spinner(LinearLayout parent, final int featNum, final String featName, final String list, String icon, int iconColor) {
        final List<String> items = new LinkedList<>(Arrays.asList(list.split(",")));
        int sel = Preferences.loadPrefInt(featName, featNum);
        if (sel >= items.size()) sel = 0;

        LinearLayout card = makeCard(true);

        LinearLayout header = new LinearLayout(getContext);
        header.setGravity(Gravity.CENTER_VERTICAL);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(iconColor);
            iconView.setTextSize(16f);
            iconView.setTypeface(iconFont);
            LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(dp(22), dp(22));
            iconParams.setMargins(0, 0, dp(8), 0);
            iconView.setLayoutParams(iconParams);
            iconView.setGravity(Gravity.CENTER);
            header.addView(iconView);
        }

        TextView label = new TextView(getContext);
        label.setText(featName);
        label.setTextColor(TEXT_PRIMARY);
        label.setTextSize(14f);
        label.setTypeface(menuFont);
        header.addView(label);

        final TextView dropdown = new TextView(getContext);
        LinearLayout.LayoutParams ddParams = new LinearLayout.LayoutParams(MATCH_PARENT, dp(44));
        ddParams.topMargin = dp(10);
        dropdown.setLayoutParams(ddParams);
        dropdown.setText(items.get(sel));
        dropdown.setTextColor(TEXT_PRIMARY);
        dropdown.setTextSize(14f);
        dropdown.setTypeface(menuFont);
        dropdown.setGravity(Gravity.CENTER_VERTICAL);
        dropdown.setPadding(dp(14), 0, dp(14), 0);

        GradientDrawable ddBg = new GradientDrawable();
        ddBg.setCornerRadius(dp(RADIUS_M));
        ddBg.setColor(BG_INPUT);
        dropdown.setBackground(ddBg);

        final int[] selected = {sel};

        dropdown.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					showSpinnerDialog(featName, featNum, items, selected, dropdown);
				}
			});

        card.addView(header);
        card.addView(dropdown);
        parent.addView(card);
    }

    private void InputNum(LinearLayout parent, final int featNum, final String featName, final int maxVal, final String icon, final int iconColor) {
        int num = Preferences.loadPrefInt(featName, featNum);
        final LinearLayout btn = createInputButton(featName, String.valueOf(num), icon, iconColor);
        final boolean hasIcon = (icon != null);

        btn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					showNumberDialog(featName, featNum, maxVal, false, new OnInputResult() {
							@Override
							public void onResult(String val) {
								updateInputButtonText(btn, featName, val, hasIcon);
							}
						});
				}
			});

        parent.addView(btn);
    }

    private void InputLNum(LinearLayout parent, final int featNum, final String featName, final long maxVal, final String icon, final int iconColor) {
        long num = Preferences.loadPrefLong(featName, featNum);
        final LinearLayout btn = createInputButton(featName, String.valueOf(num), icon, iconColor);
        final boolean hasIcon = (icon != null);

        btn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					showNumberDialog(featName, featNum, maxVal, true, new OnInputResult() {
							@Override
							public void onResult(String val) {
								updateInputButtonText(btn, featName, val, hasIcon);
							}
						});
				}
			});

        parent.addView(btn);
    }

    private void InputText(LinearLayout parent, final int featNum, final String featName, final String icon, final int iconColor) {
        String txt = Preferences.loadPrefString(featName, featNum);
        if (txt == null) txt = "";
        final LinearLayout btn = createInputButton(featName, txt, icon, iconColor);
        final boolean hasIcon = (icon != null);

        btn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					showTextDialog(featName, featNum, new OnInputResult() {
							@Override
							public void onResult(String val) {
								updateInputButtonText(btn, featName, val, hasIcon);
							}
						});
				}
			});

        parent.addView(btn);
    }

    private LinearLayout createInputButton(String name, String val, String icon, int iconColor) {
        LinearLayout btn = new LinearLayout(getContext);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH_PARENT, dp(48));
        params.setMargins(0, dp(ITEM_SPACING), 0, dp(ITEM_SPACING));
        btn.setLayoutParams(params);
        btn.setGravity(Gravity.CENTER_VERTICAL);
        btn.setOrientation(LinearLayout.HORIZONTAL);
        btn.setPadding(dp(14), 0, dp(14), 0);

        GradientDrawable bg = new GradientDrawable();
        bg.setCornerRadius(dp(RADIUS_L));
        bg.setColor(BG_CARD);
        bg.setStroke(dp(1), BORDER_SOFT);
        btn.setBackground(bg);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(iconColor);
            iconView.setTextSize(16f);
            iconView.setTypeface(iconFont);
            iconView.setPadding(0, 0, dp(10), 0);
            btn.addView(iconView);
        }

        TextView textView = new TextView(getContext);
        textView.setLayoutParams(new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f));
        if (val == null || val.isEmpty()) {
            textView.setText(Html.fromHtml(name + ": <font color='#D4A5C9'><b>-</b></font>"));
        } else {
            textView.setText(Html.fromHtml(name + ": <font color='#D4A5C9'><b>" + val + "</b></font>"));
        }
        textView.setTextColor(TEXT_PRIMARY);
        textView.setTextSize(14f);
        textView.setTypeface(menuFont);
        btn.addView(textView);

        return btn;
    }

    private void updateInputButtonText(LinearLayout btn, String name, String val, boolean hasIcon) {
        int textIndex = hasIcon ? 1 : 0;
        if (btn.getChildCount() > textIndex) {
            View child = btn.getChildAt(textIndex);
            if (child instanceof TextView) {
                if (val == null || val.isEmpty()) {
                    ((TextView) child).setText(Html.fromHtml(name + ": <font color='#D4A5C9'><b>-</b></font>"));
                } else {
                    ((TextView) child).setText(Html.fromHtml(name + ": <font color='#D4A5C9'><b>" + val + "</b></font>"));
                }
            }
        }
    }

    private void CheckBox(LinearLayout parent, final int featNum, final String featName, boolean defaultOn, String icon, final int iconColor) {
        LinearLayout card = makeCard(false);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(iconColor);
            iconView.setTextSize(16f);
            iconView.setTypeface(iconFont);
            LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(dp(22), dp(22));
            iconParams.setMargins(0, 0, dp(8), 0);
            iconView.setLayoutParams(iconParams);
            iconView.setGravity(Gravity.CENTER);
            card.addView(iconView);
        }

        final View checkBox = new View(getContext);
        LinearLayout.LayoutParams cbParams = new LinearLayout.LayoutParams(dp(22), dp(22));
        cbParams.setMargins(0, 0, dp(12), 0);
        checkBox.setLayoutParams(cbParams);

        TextView label = new TextView(getContext);
        label.setText(featName);
        label.setTextColor(TEXT_PRIMARY);
        label.setTextSize(14f);
        label.setTypeface(menuFont);
        label.setLayoutParams(new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f));

        final boolean[] checked = {Preferences.loadPrefBool(featName, featNum, defaultOn)};
        final int checkColor = (icon != null) ? iconColor : ACCENT_PURPLE;
        applyCheckStyle(checkBox, checked[0], checkColor);

        card.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					checked[0] = !checked[0];
					applyCheckStyle(checkBox, checked[0], checkColor);
					Preferences.changeFeatureBool(featName, featNum, checked[0]);
					checkBox.animate().scaleX(0.8f).scaleY(0.8f).setDuration(100)
						.withEndAction(new Runnable() {
							@Override public void run() {
								checkBox.animate().scaleX(1f).scaleY(1f).setDuration(100).start();
							}
						}).start();
				}
			});

        card.addView(checkBox);
        card.addView(label);
        parent.addView(card);
    }

    private void applyCheckStyle(View box, boolean checked, int accentColor) {
        GradientDrawable bg = new GradientDrawable();
        bg.setCornerRadius(dp(6));
        if (checked) {
            bg.setColor(accentColor);
        } else {
            bg.setColor(Color.TRANSPARENT);
            bg.setStroke(dp(2), TEXT_MUTED);
        }
        box.setBackground(bg);
    }

    private void RadioButton(LinearLayout parent, final int featNum, String featName, final String list, String icon, final int iconColor) {
        final List<String> items = new LinkedList<>(Arrays.asList(list.split(",")));

        LinearLayout card = makeCard(true);

        LinearLayout header = new LinearLayout(getContext);
        header.setGravity(Gravity.CENTER_VERTICAL);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(iconColor);
            iconView.setTextSize(16f);
            iconView.setTypeface(iconFont);
            LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(dp(22), dp(22));
            iconParams.setMargins(0, 0, dp(8), 0);
            iconView.setLayoutParams(iconParams);
            iconView.setGravity(Gravity.CENTER);
            header.addView(iconView);
        }

        final TextView label = new TextView(getContext);
        label.setText(featName + ":");
        label.setTextColor(TEXT_PRIMARY);
        label.setTextSize(14f);
        label.setTypeface(menuFont);
        header.addView(label);

        LinearLayout radioGroup = new LinearLayout(getContext);
        radioGroup.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams rgParams = new LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT);
        rgParams.topMargin = dp(8);
        radioGroup.setLayoutParams(rgParams);

        final int[] selected = {Preferences.loadPrefInt(featName, featNum)};
        final View[] radios = new View[items.size()];
        final int radioColor = (icon != null) ? iconColor : ACCENT_PINK;

        for (int i = 0; i < items.size(); i++) {
            final int idx = i;
            final String item = items.get(i);

            LinearLayout row = new LinearLayout(getContext);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setPadding(0, dp(8), 0, dp(8));

            View radio = new View(getContext);
            LinearLayout.LayoutParams rParams = new LinearLayout.LayoutParams(dp(18), dp(18));
            rParams.setMargins(0, 0, dp(12), 0);
            radio.setLayoutParams(rParams);
            radios[i] = radio;
            applyRadioStyle(radio, selected[0] == i + 1, radioColor);

            TextView itemLabel = new TextView(getContext);
            itemLabel.setText(item);
            itemLabel.setTextColor(TEXT_SECONDARY);
            itemLabel.setTextSize(13f);
            itemLabel.setTypeface(menuFont);

            final String fName = featName;
            row.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						selected[0] = idx + 1;
						for (int j = 0; j < radios.length; j++) {
							applyRadioStyle(radios[j], j == idx, radioColor);
						}
						label.setText(Html.fromHtml(fName + ": <font color='#D4A5C9'>" + item + "</font>"));
						Preferences.changeFeatureInt(fName, featNum, selected[0]);
					}
				});

            row.addView(radio);
            row.addView(itemLabel);
            radioGroup.addView(row);
        }

        if (selected[0] > 0 && selected[0] <= items.size()) {
            label.setText(Html.fromHtml(featName + ": <font color='#D4A5C9'>" + items.get(selected[0] - 1) + "</font>"));
        }

        card.addView(header);
        card.addView(radioGroup);
        parent.addView(card);
    }

    private void applyRadioStyle(View radio, boolean selected, int accentColor) {
        GradientDrawable bg = new GradientDrawable();
        bg.setShape(GradientDrawable.OVAL);
        if (selected) {
            bg.setColor(accentColor);
            bg.setStroke(dp(2), accentColor);
        } else {
            bg.setColor(Color.TRANSPARENT);
            bg.setStroke(dp(2), TEXT_MUTED);
        }
        radio.setBackground(bg);
    }

    private void Collapse(LinearLayout parent, final String text, final boolean expanded, String icon, int bgColor) {
        final LinearLayout header = new LinearLayout(getContext);
        header.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams hParams = new LinearLayout.LayoutParams(MATCH_PARENT, dp(46));
        hParams.setMargins(0, dp(ITEM_SPACING), 0, 0);
        header.setLayoutParams(hParams);
        header.setPadding(dp(14), 0, dp(14), 0);

        int headerColor = (icon != null) ? bgColor : ACCENT_PEACH;

        GradientDrawable hBg = new GradientDrawable();
        hBg.setCornerRadius(dp(RADIUS_L));
        hBg.setColor(headerColor);
        header.setBackground(hBg);

        final TextView arrow = new TextView(getContext);
        arrow.setText(expanded ? "\uEA4D" : "\uEA6D");
        arrow.setTextColor(BG_PRIMARY);
        arrow.setTextSize(18f);
        arrow.setTypeface(iconFont);
        header.addView(arrow);

        TextView title = new TextView(getContext);
        title.setText(text);
        title.setTextColor(BG_PRIMARY);
        title.setTextSize(14f);
        title.setTypeface(menuFont, Typeface.BOLD);
        title.setPadding(dp(8), 0, 0, 0);
        header.addView(title);

        final LinearLayout content = new LinearLayout(getContext);
        content.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams cParams = new LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT);
        cParams.setMargins(0, dp(2), 0, dp(ITEM_SPACING));
        content.setLayoutParams(cParams);
        content.setPadding(dp(12), dp(10), dp(12), dp(10));
        content.setVisibility(expanded ? View.VISIBLE : View.GONE);

        GradientDrawable cBg = new GradientDrawable();
        cBg.setCornerRadii(new float[]{0, 0, 0, 0, dp(RADIUS_L), dp(RADIUS_L), dp(RADIUS_L), dp(RADIUS_L)});
        cBg.setColor(BG_CARD_LIGHT);
        content.setBackground(cBg);

        mCollapse = content;

        final boolean[] isOpen = {expanded};
        header.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					isOpen[0] = !isOpen[0];
					arrow.setText(isOpen[0] ? "\uEA4D" : "\uEA6D");
					if (isOpen[0]) {
						content.setVisibility(View.VISIBLE);
						content.setAlpha(0f);
						content.animate().alpha(1f).setDuration(200).start();
					} else {
						content.animate().alpha(0f).setDuration(150)
							.withEndAction(new Runnable() {
								@Override public void run() {
									content.setVisibility(View.GONE);
									content.setAlpha(1f);
								}
							}).start();
					}
				}
			});

        parent.addView(header);
        parent.addView(content);
    }

    private void Category(LinearLayout parent, String text, String icon, int iconColor) {
        LinearLayout container = new LinearLayout(getContext);
        container.setOrientation(LinearLayout.HORIZONTAL);
        container.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT);
        params.setMargins(0, dp(14), 0, dp(6));
        container.setLayoutParams(params);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(iconColor);
            iconView.setTextSize(12f);
            iconView.setTypeface(iconFont);
            iconView.setPadding(0, 0, dp(6), 0);
            container.addView(iconView);
        }

        int catColor = (icon != null) ? iconColor : ACCENT_PURPLE;

        TextView cat = new TextView(getContext);
        cat.setText(Html.fromHtml(text));
        cat.setTextColor(catColor);
        cat.setTextSize(11f);
        cat.setTypeface(menuFont, Typeface.BOLD);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            cat.setLetterSpacing(0.08f);
        }
        container.addView(cat);

        parent.addView(container);
    }

    private void TextView(LinearLayout parent, String text, String icon, int iconColor) {
        LinearLayout container = new LinearLayout(getContext);
        container.setOrientation(LinearLayout.HORIZONTAL);
        container.setGravity(Gravity.TOP);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT);
        params.setMargins(0, dp(4), 0, dp(4));
        container.setLayoutParams(params);

        if (icon != null) {
            TextView iconView = new TextView(getContext);
            iconView.setText(icon);
            iconView.setTextColor(iconColor);
            iconView.setTextSize(14f);
            iconView.setTypeface(iconFont);
            iconView.setPadding(0, dp(2), dp(8), 0);
            container.addView(iconView);
        }

        TextView tv = new TextView(getContext);
        tv.setLayoutParams(new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f));
        tv.setText(Html.fromHtml(text));
        tv.setTextColor(TEXT_SECONDARY);
        tv.setTextSize(13f);
        tv.setTypeface(menuFont);
        tv.setLineSpacing(0, 1.2f);
        container.addView(tv);

        parent.addView(container);
    }

    interface OnInputResult { void onResult(String value); }

    private void showNumberDialog(final String title, final int featNum, final long maxVal, final boolean isLong, final OnInputResult callback) {
        final FrameLayout overlay = createDialogOverlay();
        LinearLayout dialog = createDialogBox(title, ACCENT_PINK);

        final EditText input = new EditText(getContext);
        LinearLayout.LayoutParams iParams = new LinearLayout.LayoutParams(MATCH_PARENT, dp(50));
        iParams.setMargins(0, dp(16), 0, dp(16));
        input.setLayoutParams(iParams);
        input.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED);
        input.setTextColor(TEXT_PRIMARY);
        input.setHintTextColor(TEXT_MUTED);
        input.setHint(maxVal > 0 ? "Max: " + maxVal : "Enter number...");
        input.setTextSize(16f);
        input.setTypeface(menuFont);
        input.setGravity(Gravity.CENTER);
        input.setPadding(dp(14), dp(12), dp(14), dp(12));

        GradientDrawable iBg = new GradientDrawable();
        iBg.setCornerRadius(dp(RADIUS_M));
        iBg.setColor(BG_INPUT);
        input.setBackground(iBg);

        LinearLayout btns = createDialogButtons(
            new View.OnClickListener() {
                @Override public void onClick(View v) { removeDialogOverlay(overlay); }
            },
            new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    String txt = input.getText().toString().trim();
                    long val = 0;
                    try {
                        if (!txt.isEmpty()) val = Long.parseLong(txt);
                        if (maxVal > 0 && val > maxVal) val = maxVal;
                    } catch (NumberFormatException e) {
                        val = maxVal > 0 ? maxVal : (isLong ? Long.MAX_VALUE : Integer.MAX_VALUE);
                    }
                    if (isLong) Preferences.changeFeatureLong(title, featNum, val);
                    else Preferences.changeFeatureInt(title, featNum, (int) val);
                    if (callback != null) callback.onResult(String.valueOf(val));
                    removeDialogOverlay(overlay);
                }
            },
            ACCENT_PINK
        );

        dialog.addView(input);
        dialog.addView(btns);
        showDialogOverlay(overlay, dialog);
    }

    private void showTextDialog(final String title, final int featNum, final OnInputResult callback) {
        final FrameLayout overlay = createDialogOverlay();
        LinearLayout dialog = createDialogBox(title, ACCENT_BLUE);

        final EditText input = new EditText(getContext);
        LinearLayout.LayoutParams iParams = new LinearLayout.LayoutParams(MATCH_PARENT, dp(50));
        iParams.setMargins(0, dp(16), 0, dp(16));
        input.setLayoutParams(iParams);
        input.setInputType(InputType.TYPE_CLASS_TEXT);
        input.setTextColor(TEXT_PRIMARY);
        input.setHintTextColor(TEXT_MUTED);
        input.setHint("Enter text...");
        input.setTextSize(16f);
        input.setTypeface(menuFont);
        input.setGravity(Gravity.CENTER);
        input.setPadding(dp(14), dp(12), dp(14), dp(12));

        String currentVal = Preferences.loadPrefString(title, featNum);
        if (currentVal != null && !currentVal.isEmpty()) input.setText(currentVal);

        GradientDrawable iBg = new GradientDrawable();
        iBg.setCornerRadius(dp(RADIUS_M));
        iBg.setColor(BG_INPUT);
        input.setBackground(iBg);

        LinearLayout btns = createDialogButtons(
            new View.OnClickListener() {
                @Override public void onClick(View v) { removeDialogOverlay(overlay); }
            },
            new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    String txt = input.getText().toString();
                    Preferences.changeFeatureString(title, featNum, txt);
                    if (callback != null) callback.onResult(txt);
                    removeDialogOverlay(overlay);
                }
            },
            ACCENT_BLUE
        );

        dialog.addView(input);
        dialog.addView(btns);
        showDialogOverlay(overlay, dialog);
    }

    private void showSpinnerDialog(final String title, final int featNum, final List<String> items, final int[] selected, final TextView dropdown) {
        final FrameLayout overlay = createDialogOverlay();
        LinearLayout dialog = createDialogBox(title, ACCENT_PURPLE);

        ScrollView sv = new ScrollView(getContext);
        LinearLayout.LayoutParams svParams = new LinearLayout.LayoutParams(MATCH_PARENT, dp(200));
        svParams.setMargins(0, dp(12), 0, 0);
        sv.setLayoutParams(svParams);
        sv.setVerticalScrollBarEnabled(true);

        LinearLayout list = new LinearLayout(getContext);
        list.setOrientation(LinearLayout.VERTICAL);
        list.setPadding(0, dp(4), 0, dp(4));

        for (int i = 0; i < items.size(); i++) {
            final int idx = i;
            final String item = items.get(i);

            LinearLayout itemContainer = new LinearLayout(getContext);
            itemContainer.setOrientation(LinearLayout.HORIZONTAL);
            itemContainer.setGravity(Gravity.CENTER_VERTICAL);
            LinearLayout.LayoutParams icParams = new LinearLayout.LayoutParams(MATCH_PARENT, dp(44));
            icParams.setMargins(0, dp(2), 0, dp(2));
            itemContainer.setLayoutParams(icParams);
            itemContainer.setPadding(dp(14), 0, dp(14), 0);

            boolean isSelected = (selected[0] == i);

            GradientDrawable icBg = new GradientDrawable();
            icBg.setCornerRadius(dp(RADIUS_M));
            icBg.setColor(isSelected ? Color.parseColor("#33A99FD3") : BG_CARD_LIGHT);
            itemContainer.setBackground(icBg);

            TextView checkIcon = new TextView(getContext);
            checkIcon.setText("\uEB7A");
            checkIcon.setTextColor(ACCENT_PURPLE);
            checkIcon.setTextSize(14f);
            checkIcon.setTypeface(iconFont);
            checkIcon.setVisibility(isSelected ? View.VISIBLE : View.INVISIBLE);
            LinearLayout.LayoutParams checkParams = new LinearLayout.LayoutParams(dp(24), dp(24));
            checkParams.setMargins(0, 0, dp(8), 0);
            checkIcon.setLayoutParams(checkParams);
            checkIcon.setGravity(Gravity.CENTER);
            itemContainer.addView(checkIcon);

            TextView itemView = new TextView(getContext);
            itemView.setText(item);
            itemView.setTextColor(isSelected ? ACCENT_PURPLE : TEXT_PRIMARY);
            itemView.setTextSize(14f);
            itemView.setTypeface(menuFont);
            itemView.setLayoutParams(new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f));
            itemContainer.addView(itemView);

            itemContainer.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						selected[0] = idx;
						dropdown.setText(item);
						Preferences.changeFeatureInt(title, featNum, idx);
						removeDialogOverlay(overlay);
					}
				});

            list.addView(itemContainer);
        }

        sv.addView(list);
        dialog.addView(sv);
        showDialogOverlay(overlay, dialog);

        overlay.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v) { removeDialogOverlay(overlay); }
			});
    }

    private FrameLayout createDialogOverlay() {
        FrameLayout overlay = new FrameLayout(getContext);
        overlay.setLayoutParams(new FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT));
        overlay.setBackgroundColor(Color.parseColor("#CC000000"));
        overlay.setClickable(true);
        return overlay;
    }

    private LinearLayout createDialogBox(String title, int accentColor) {
        LinearLayout box = new LinearLayout(getContext);
        box.setOrientation(LinearLayout.VERTICAL);
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(dp(280), WRAP_CONTENT);
        params.gravity = Gravity.CENTER;
        box.setLayoutParams(params);
        box.setPadding(dp(20), dp(20), dp(20), dp(20));
        box.setClickable(true);

        GradientDrawable bg = new GradientDrawable();
        bg.setCornerRadius(dp(RADIUS_XL));
        bg.setColor(BG_PRIMARY);
        bg.setStroke(dp(2), accentColor);
        box.setBackground(bg);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            box.setElevation(dp(20));
        }

        TextView titleView = new TextView(getContext);
        titleView.setText(title);
        titleView.setTextColor(accentColor);
        titleView.setTextSize(16f);
        titleView.setTypeface(menuFont, Typeface.BOLD);
        titleView.setGravity(Gravity.CENTER);
        box.addView(titleView);

        return box;
    }

    private LinearLayout createDialogButtons(View.OnClickListener onCancel, View.OnClickListener onOk, int okColor) {
        LinearLayout btns = new LinearLayout(getContext);
        btns.setOrientation(LinearLayout.HORIZONTAL);
        btns.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams btnsParams = new LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT);
        btnsParams.topMargin = dp(8);
        btns.setLayoutParams(btnsParams);

        LinearLayout.LayoutParams bParams = new LinearLayout.LayoutParams(0, dp(42), 1);
        bParams.setMargins(dp(4), 0, dp(4), 0);

        LinearLayout cancelBtn = new LinearLayout(getContext);
        cancelBtn.setLayoutParams(bParams);
        cancelBtn.setGravity(Gravity.CENTER);
        cancelBtn.setOrientation(LinearLayout.HORIZONTAL);

        GradientDrawable cBg = new GradientDrawable();
        cBg.setCornerRadius(dp(RADIUS_M));
        cBg.setColor(BG_CARD);
        cancelBtn.setBackground(cBg);

        TextView cancelIcon = new TextView(getContext);
        cancelIcon.setText("\uE4F6");
        cancelIcon.setTextColor(TEXT_SECONDARY);
        cancelIcon.setTextSize(14f);
        cancelIcon.setTypeface(iconFont);
        cancelIcon.setPadding(0, 0, dp(6), 0);
        cancelBtn.addView(cancelIcon);

        TextView cancelText = new TextView(getContext);
        cancelText.setText("Cancel");
        cancelText.setTextColor(TEXT_SECONDARY);
        cancelText.setTextSize(13f);
        cancelText.setTypeface(menuFont);
        cancelBtn.addView(cancelText);

        cancelBtn.setOnClickListener(onCancel);

        LinearLayout okBtn = new LinearLayout(getContext);
        okBtn.setLayoutParams(bParams);
        okBtn.setGravity(Gravity.CENTER);
        okBtn.setOrientation(LinearLayout.HORIZONTAL);

        GradientDrawable oBg = new GradientDrawable();
        oBg.setCornerRadius(dp(RADIUS_M));
        oBg.setColor(okColor);
        okBtn.setBackground(oBg);

        TextView okIcon = new TextView(getContext);
        okIcon.setText("\uEB7A");
        okIcon.setTextColor(BG_PRIMARY);
        okIcon.setTextSize(14f);
        okIcon.setTypeface(iconFont);
        okIcon.setPadding(0, 0, dp(6), 0);
        okBtn.addView(okIcon);

        TextView okText = new TextView(getContext);
        okText.setText("OK");
        okText.setTextColor(BG_PRIMARY);
        okText.setTextSize(13f);
        okText.setTypeface(menuFont, Typeface.BOLD);
        okBtn.addView(okText);

        okBtn.setOnClickListener(onOk);

        btns.addView(cancelBtn);
        btns.addView(okBtn);

        return btns;
    }

    @SuppressLint("WrongConstant")
    private void showDialogOverlay(final FrameLayout overlay, final LinearLayout dialog) {
        overlay.addView(dialog);
        overlay.setAlpha(0f);
        dialog.setScaleX(0.85f);
        dialog.setScaleY(0.85f);

        if (overlayRequired) {
            int type = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ?
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
                WindowManager.LayoutParams.TYPE_PHONE;

            WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                MATCH_PARENT, MATCH_PARENT, type,
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL |
                WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH,
                PixelFormat.TRANSLUCENT
            );
            mWindowManager.addView(overlay, params);
        } else {
            ((Activity) getContext).addContentView(overlay, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
        }

        overlay.animate().alpha(1f).setDuration(200).start();
        dialog.animate().scaleX(1f).scaleY(1f).setDuration(250).setInterpolator(new OvershootInterpolator(1.1f)).start();
    }

    private void removeDialogOverlay(final FrameLayout overlay) {
        overlay.animate().alpha(0f).setDuration(150)
            .withEndAction(new Runnable() {
                @Override
                public void run() {
                    try {
                        if (overlayRequired) {
                            if (mWindowManager != null && overlay.isAttachedToWindow()) {
                                mWindowManager.removeView(overlay);
                            }
                        } else {
                            ViewGroup parent = (ViewGroup) overlay.getParent();
                            if (parent != null) parent.removeView(overlay);
                        }
                    } catch (Exception e) {}
                }
            }).start();
    }

    private void featureList(String[] list, LinearLayout container) {
        int featNum, subFeat = 0;
        LinearLayout target = container;

        for (int i = 0; i < list.length; i++) {
            boolean defOn = false;
            String feature = list[i];

            if (feature.contains("_True")) {
                defOn = true;
                feature = feature.replace("_True", "");
            }

            target = container;
            if (feature.contains("CollapseAdd_")) {
                target = mCollapse;
                feature = feature.replace("CollapseAdd_", "");
            }

            String icon = extractIcon(feature);
            int iconColor = getColorForIcon(icon);
            feature = removeIconFromName(feature);

            String[] parts = feature.split("_");

            if (TextUtils.isDigitsOnly(parts[0]) || parts[0].matches("-?\\d+")) {
                featNum = Integer.parseInt(parts[0]);
                feature = feature.replaceFirst(parts[0] + "_", "");
                subFeat++;
            } else {
                featNum = i - subFeat;
            }

            String[] p = feature.split("_");

            switch (p[0]) {
                case "Toggle":
                    Switch(target, featNum, p[1], defOn, icon, iconColor);
                    break;
                case "SeekBar":
                    SeekBar(target, featNum, p[1], Integer.parseInt(p[2]), Integer.parseInt(p[3]), icon, iconColor);
                    break;
                case "Button":
                    Button(target, featNum, p[1], icon, iconColor);
                    break;
                case "ButtonOnOff":
                    ButtonOnOff(target, featNum, p[1], defOn, icon);
                    break;
                case "Spinner":
                    Spinner(target, featNum, p[1], p[2], icon, iconColor);
                    break;
                case "InputText":
                    InputText(target, featNum, p[1], icon, iconColor);
                    break;
                case "InputValue":
                    if (p.length == 3) InputNum(target, featNum, p[2], Integer.parseInt(p[1]), icon, iconColor);
                    else InputNum(target, featNum, p[1], 0, icon, iconColor);
                    break;
                case "InputLValue":
                    if (p.length == 3) InputLNum(target, featNum, p[2], Long.parseLong(p[1]), icon, iconColor);
                    else InputLNum(target, featNum, p[1], 0, icon, iconColor);
                    break;
                case "CheckBox":
                    CheckBox(target, featNum, p[1], defOn, icon, iconColor);
                    break;
                case "RadioButton":
                    RadioButton(target, featNum, p[1], p[2], icon, iconColor);
                    break;
                case "Collapse":
                    Collapse(target, p[1], defOn, icon, iconColor);
                    subFeat++;
                    break;
                case "ButtonLink":
                    ButtonLink(target, p[1], p[2], icon, iconColor);
                    subFeat++;
                    break;
                case "Category":
                    Category(target, p[1], icon, iconColor);
                    subFeat++;
                    break;
                case "RichTextView":
                    TextView(target, p[1], icon, iconColor);
                    subFeat++;
                    break;
            }
        }
    }

    public void ShowMenu() {
        rootFrame.addView(mRootContainer);
        initESP();

        final Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
				boolean loaded = false;
				@Override
				public void run() {
					if (Preferences.loadPref && !IsGameLibLoaded() && !stopChecking) {
						if (!loaded) {
							Category(mods, "LOADING", "\uF080", ACCENT_BLUE);
							TextView(mods, "Waiting for game library...", "\uF210", TEXT_MUTED);
							Button(mods, -100, "Force Load", "\uED3D", ACCENT_PEACH);
							loaded = true;
						}
						handler.postDelayed(this, 500);
					} else {
						mods.removeAllViews();
						featureList(GetFeatureList(), mods);
					}
				}
			}, 300);
    }

    @SuppressLint("WrongConstant")
    public void SetWindowManagerWindowService() {
        int type = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ?
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
            WindowManager.LayoutParams.TYPE_PHONE;

        vmParams = new WindowManager.LayoutParams(
            WRAP_CONTENT, WRAP_CONTENT, type,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        vmParams.gravity = Gravity.TOP | Gravity.START;
        vmParams.x = POS_X;
        vmParams.y = POS_Y;

        mWindowManager = (WindowManager) getContext.getSystemService(Context.WINDOW_SERVICE);
        mWindowManager.addView(rootFrame, vmParams);

        mCollapsed.setOnTouchListener(iconDragListener());
        overlayRequired = true;
    }

    @SuppressLint("WrongConstant")
    public void SetWindowManagerActivity() {
        vmParams = new WindowManager.LayoutParams(
            WRAP_CONTENT, WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        vmParams.gravity = Gravity.TOP | Gravity.START;
        vmParams.x = POS_X;
        vmParams.y = POS_Y;

        mWindowManager = ((Activity) getContext).getWindowManager();
        mWindowManager.addView(rootFrame, vmParams);

        mCollapsed.setOnTouchListener(iconDragListener());
    }

    private View.OnTouchListener iconDragListener() {
        return new View.OnTouchListener() {
            private int startX, startY;
            private float touchX, touchY;
            private boolean dragging = false;
            private static final int CLICK_THRESHOLD = 10;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        startX = vmParams.x;
                        startY = vmParams.y;
                        touchX = event.getRawX();
                        touchY = event.getRawY();
                        dragging = false;
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        int dx = (int) (event.getRawX() - touchX);
                        int dy = (int) (event.getRawY() - touchY);
                        if (Math.abs(dx) > CLICK_THRESHOLD || Math.abs(dy) > CLICK_THRESHOLD) dragging = true;
                        if (dragging) {
                            vmParams.x = startX + dx;
                            vmParams.y = startY + dy;
                            mWindowManager.updateViewLayout(rootFrame, vmParams);
                        }
                        return true;
                    case MotionEvent.ACTION_UP:
                        if (!dragging) {
                            mCollapsed.setVisibility(View.GONE);
                            mExpanded.setVisibility(View.VISIBLE);
                            mExpanded.setAlpha(0f);
                            mExpanded.setScaleX(0.9f);
                            mExpanded.setScaleY(0.9f);
                            mExpanded.animate()
                                .alpha(1f).scaleX(1f).scaleY(1f)
                                .setDuration(250)
                                .setInterpolator(new OvershootInterpolator(1.1f))
                                .start();
                        }
                        return true;
                }
                return false;
            }
        };
    }

    private View.OnTouchListener menuDragListener() {
        return new View.OnTouchListener() {
            private int startX, startY;
            private float touchX, touchY;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        startX = vmParams.x;
                        startY = vmParams.y;
                        touchX = event.getRawX();
                        touchY = event.getRawY();
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        vmParams.x = startX + (int) (event.getRawX() - touchX);
                        vmParams.y = startY + (int) (event.getRawY() - touchY);
                        mWindowManager.updateViewLayout(rootFrame, vmParams);
                        return true;
                    case MotionEvent.ACTION_UP:
                        return true;
                }
                return false;
            }
        };
    }

    private int dp(int val) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, val, getContext.getResources().getDisplayMetrics());
    }

    public void setVisibility(int visibility) {
        if (rootFrame != null) rootFrame.setVisibility(visibility);
    }

    public void onDestroy() {
        destroyESP();
        if (rootFrame != null && mWindowManager != null) {
            try { mWindowManager.removeView(rootFrame); } catch (Exception e) {}
        }
    }

    public static Menu getInstance() { return instance; }
    public Typeface getMenuFont() { return menuFont; }
    public Typeface getIconFont() { return iconFont; }
    public boolean areFontsLoaded() { return fontsLoaded; }
}
