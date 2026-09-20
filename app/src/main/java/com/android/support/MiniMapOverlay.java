package com.android.support;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class MiniMapOverlay {

    private final Context context;
    private final WindowManager windowManager;
    private WindowManager.LayoutParams params;

    private RelativeLayout rootLayout;
    private MiniMapView miniMapView;
    private volatile boolean isAttached = false;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    private int currentWidth = 150;
    private int currentHeight = 150;
    private static final int MIN_SIZE = 110;
    private static final int MAX_SIZE = 450;

    public MiniMapOverlay(Context ctx) {
        this.context = ctx;
        this.windowManager = (WindowManager) ctx.getSystemService(Context.WINDOW_SERVICE);
        buildView();
    }

    private void buildView() {
        int type = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ?
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
                WindowManager.LayoutParams.TYPE_PHONE;

        params = new WindowManager.LayoutParams(
                dp(currentWidth),
                dp(currentHeight + 20),
                type,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                        WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                PixelFormat.TRANSLUCENT
        );
        params.gravity = Gravity.TOP | Gravity.END;
        params.x = 0;
        params.y = 0;

        rootLayout = new RelativeLayout(context);

        GradientDrawable bg = new GradientDrawable();
        bg.setColor(Color.parseColor("#D916141F"));
        bg.setCornerRadius(dp(14));
        bg.setStroke(dp(1), Color.parseColor("#44D4A5C9"));
        rootLayout.setBackground(bg);
        rootLayout.setPadding(dp(4), dp(4), dp(4), dp(4));

        miniMapView = new MiniMapView(context);
        RelativeLayout.LayoutParams mapParams = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT,
                RelativeLayout.LayoutParams.MATCH_PARENT
        );
        mapParams.setMargins(0, 0, 0, dp(18));
        miniMapView.setLayoutParams(mapParams);

        GradientDrawable mapClip = new GradientDrawable();
        mapClip.setCornerRadius(dp(10));
        mapClip.setColor(Color.BLACK);
        miniMapView.setBackground(mapClip);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            miniMapView.setClipToOutline(true);
        }
        rootLayout.addView(miniMapView);

        FrameLayout dragBar = new FrameLayout(context);
        RelativeLayout.LayoutParams barParams = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT, dp(18)
        );
        barParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        dragBar.setLayoutParams(barParams);

        View line = new View(context);
        FrameLayout.LayoutParams lineParams = new FrameLayout.LayoutParams(dp(40), dp(3));
        lineParams.gravity = Gravity.CENTER;
        line.setLayoutParams(lineParams);

        GradientDrawable lineBg = new GradientDrawable();
        lineBg.setColor(Color.parseColor("#88FFFFFF"));
        lineBg.setCornerRadius(dp(2));
        line.setBackground(lineBg);

        dragBar.addView(line);
        dragBar.setOnTouchListener(createMoveListener());
        rootLayout.addView(dragBar);

        TextView resizeHandle = new TextView(context);
        resizeHandle.setText("\uE0A6");
        resizeHandle.setTextColor(Color.parseColor("#B8B5C8"));
        resizeHandle.setTextSize(14f);
        try {
            resizeHandle.setTypeface(Typeface.createFromAsset(context.getAssets(), "fonts/Phosphor-Bold.ttf"));
        } catch (Exception ignored) {}

        RelativeLayout.LayoutParams resizeParams = new RelativeLayout.LayoutParams(dp(24), dp(24));
        resizeParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        resizeParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        resizeHandle.setLayoutParams(resizeParams);
        resizeHandle.setGravity(Gravity.CENTER);
        resizeHandle.setOnTouchListener(createResizeListener());
        rootLayout.addView(resizeHandle);
    }

    private View.OnTouchListener createMoveListener() {
        return new View.OnTouchListener() {
            private int initialX, initialY;
            private float initialTouchX, initialTouchY;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = params.x;
                        initialY = params.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        params.x = initialX - (int) (event.getRawX() - initialTouchX);
                        params.y = initialY + (int) (event.getRawY() - initialTouchY);
                        if (isAttached) windowManager.updateViewLayout(rootLayout, params);
                        return true;
                }
                return false;
            }
        };
    }

    private View.OnTouchListener createResizeListener() {
        return new View.OnTouchListener() {
            private int initialW, initialH;
            private float initialTouchX, initialTouchY;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialW = params.width;
                        initialH = params.height;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        int delta = (int) ((initialTouchX - event.getRawX()) + (event.getRawY() - initialTouchY)) / 2;
                        int newSize = Math.max(dp(MIN_SIZE), Math.min(dp(MAX_SIZE), initialW + delta));
                        params.width = newSize;
                        params.height = newSize + dp(18);
                        if (isAttached) windowManager.updateViewLayout(rootLayout, params);
                        return true;
                }
                return false;
            }
        };
    }

    public void setMapId(int id) {
        if (miniMapView != null) miniMapView.setMapByEnum(id);
    }

    public void parseAndUpdateData(String data) {
        if (!isAttached || miniMapView == null || data == null || data.isEmpty()) return;

        List<MiniMapView.MapEntity> list = new ArrayList<>();
        String[] entries = data.split(";");
        for (String entry : entries) {
            if (entry.isEmpty()) continue;
            String[] p = entry.split(",");
            if (p.length >= 5) {
                try {
                    float x = Float.parseFloat(p[0]);
                    float y = Float.parseFloat(p[1]);
                    boolean dead = p[2].equals("1");
                    boolean local = p[3].equals("1");
                    int colorId = Integer.parseInt(p[4]);
                    String name = (p.length >= 6) ? p[5] : "";
                    list.add(new MiniMapView.MapEntity(x, y, dead, local, colorId, name));
                } catch (Exception ignored) {}
            }
        }
        miniMapView.updateEntities(list);
    }

    public void setVisible(final boolean show) {
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (show) {
                    if (!isAttached && rootLayout != null) {
                        try {
                            windowManager.addView(rootLayout, params);
                            isAttached = true;
                        } catch (Exception ignored) {}
                    }
                } else {
                    if (isAttached && rootLayout != null) {
                        try {
                            windowManager.removeView(rootLayout);
                            isAttached = false;
                        } catch (Exception ignored) {}
                    }
                }
            }
        });
    }

    public void show() { setVisible(true); }
    public void hide() { setVisible(false); }
    public boolean isShowing() { return isAttached; }
    public boolean isAttached() { return isAttached; }

    private int dp(int val) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, val, context.getResources().getDisplayMetrics());
    }
}