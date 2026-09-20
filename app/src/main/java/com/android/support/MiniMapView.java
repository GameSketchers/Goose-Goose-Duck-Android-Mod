package com.android.support;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.View;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MiniMapView extends View {

    public static class MapEntity {
        public float worldX, worldY;
        public boolean isDead;
        public boolean isLocal;
        public int colorId;
        public String name;

        public MapEntity(float x, float y, boolean dead, boolean local, int cId, String n) {
            this.worldX = x;
            this.worldY = y;
            this.isDead = dead;
            this.isLocal = local;
            this.colorId = cId;
            this.name = n;
        }
    }

    private static final String[] COLOR_NAMES = {
            "red",         // 0
            "blue",        // 1
            "yellow",      // 2
            "green",       // 3
            "light_pink",  // 4
            "orange",      // 5
            "beige",       // 6
            "white",       // 7
            "dark_gray",   // 8
            "purple",      // 9
            "lime",        // 10
            "cyan",        // 11
            "pink",        // 12
            "gray",        // 13
            "brown",       // 14
            "navy",        // 15
            "olive",       // 16
            "black",       // 17
            "maroon",      // 18
            "cream"        // 19
    };

    private volatile Bitmap mapBitmap = null;
    private final Map<Integer, Bitmap> aliveIcons = new HashMap<>();
    private final Map<Integer, Bitmap> deadIcons = new HashMap<>();

    private String currentMapKey = "";

    public static boolean showPlayers = true;
    public static boolean showDeadBodies = true;
    public static boolean touchTeleportEnabled = true;

    private final Paint bgPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint bitmapPaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final Paint localRingPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    private final List<MapEntity> entities = new ArrayList<>();

    private float worldMinX = -36.0f;
    private float worldMaxX = 46.0f;
    private float worldMinY = -40.0f;
    private float worldMaxY = 22.0f;

    private final RectF renderMapRect = new RectF();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private long lastTeleportTime = 0;

    public static native void nativeDirectTeleport(float x, float y);

    public MiniMapView(Context context) {
        super(context);

        bgPaint.setColor(Color.parseColor("#E616141F"));
        bgPaint.setStyle(Paint.Style.FILL);

        localRingPaint.setStyle(Paint.Style.STROKE);
        localRingPaint.setStrokeWidth(3.5f);
        localRingPaint.setColor(Color.CYAN);

        textPaint.setColor(Color.WHITE);
        textPaint.setTextSize(20f);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setFakeBoldText(true);

        loadIcons(context);
    }

    private void loadIcons(final Context context) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < COLOR_NAMES.length; i++) {
                    String colorName = COLOR_NAMES[i];
                    final int id = i;

                    try {
                        InputStream is = context.getAssets().open("alive_gooses/alive_goose_" + colorName + ".png");
                        final Bitmap bmp = BitmapFactory.decodeStream(is);
                        is.close();
                        if (bmp != null) {
                            mainHandler.post(new Runnable() {
                                @Override public void run() { aliveIcons.put(id, bmp); }
                            });
                        }
                    } catch (Exception ignored) {}

                    try {
                        InputStream is = context.getAssets().open("dead_bodys/dead_body_" + colorName + ".png");
                        final Bitmap bmp = BitmapFactory.decodeStream(is);
                        is.close();
                        if (bmp != null) {
                            mainHandler.post(new Runnable() {
                                @Override public void run() { deadIcons.put(id, bmp); }
                            });
                        }
                    } catch (Exception ignored) {}
                }
            }
        }).start();
    }

    public void setMapByEnum(int mapId) {
        String baseName;
        switch (mapId) {
            case 0:  baseName = "goosechapel_mini_map"; break;
            case 1:  baseName = "mallard_manor_mini_map"; break;
            case 2:  baseName = "nexus_colony_mini_map"; break;
            case 3:  baseName = "black_swan_mini_map"; break;
            case 4:  baseName = "mother_goose_mini_map"; break;
            case 6:  baseName = "jungle_temple_mini_map"; break;
            case 7:  baseName = "basement_mini_map"; break;
            case 8:  baseName = "ancient_sands_mini_map"; break;
            case 9:  baseName = "bloodhaven_mini_map"; break;
            case 10: baseName = "eagleton_springs_mini_map"; break;
            case 11: baseName = "carnival_mini_map"; break;
            case 12: baseName = "mallardon_mini_map"; break;
            default: baseName = "goosechapel_mini_map"; break;
        }
        loadMapAsync(baseName);
    }

    public void setMapByName(String mapName) {
        loadMapAsync(mapName);
    }

    public void loadMapAsync(final String baseName) {
        if (baseName == null || baseName.equals(currentMapKey)) return;
        currentMapKey = baseName;
        adjustBoundsForMap(baseName);

        new Thread(new Runnable() {
            @Override
            public void run() {
                Bitmap loaded = null;
                try {
                    InputStream is = getContext().getAssets().open("maps/" + baseName + ".png");
                    loaded = BitmapFactory.decodeStream(is);
                    is.close();
                } catch (Exception ignored) {}

                final Bitmap finalLoaded = loaded;
                mainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mapBitmap != null && !mapBitmap.isRecycled()) {
                            mapBitmap.recycle();
                        }
                        mapBitmap = finalLoaded;
                        postInvalidate();
                    }
                });
            }
        }).start();
    }

    private void adjustBoundsForMap(String name) {
        switch (name) {
            // ID 0: Goosechapel (1024x768 - 4:3)
            case "goosechapel_mini_map":
                worldMinX = -36.0f; worldMaxX = 46.0f;
                worldMinY = -40.0f; worldMaxY = 22.0f;
                break;

            // ID 1: Mallard Manor (1200x900 - 4:3)
            case "mallard_manor_mini_map":
                worldMinX = -27.6f; worldMaxX = 41.7f;
                worldMinY = -42.0f; worldMaxY = 10.0f;
                break;

            // ID 2: Nexus Colony (1200x900 - 4:3)
            case "nexus_colony_mini_map":
                worldMinX = -68.0f; worldMaxX = 64.0f;
                worldMinY = -49.5f; worldMaxY = 49.5f;
                break;

            // ID 3: Black Swan (1200x900 - 4:3)
            case "black_swan_mini_map":
                worldMinX = -48.0f; worldMaxX = 48.0f;
                worldMinY = -36.0f; worldMaxY = 36.0f;
                break;

            // ID 4: S.S. Mother Goose (1200x900 - 4:3)
            case "mother_goose_mini_map":
                worldMinX = -50.0f; worldMaxX = 50.0f;
                worldMinY = -37.5f; worldMaxY = 37.5f;
                break;

            // ID 6: Jungle Temple (1024x768 - 4:3)
            case "jungle_temple_mini_map":
                worldMinX = -41.0f; worldMaxX = 45.0f;
                worldMinY = -40.0f; worldMaxY = 24.5f;
                break;

            // ID 7: The Basement (1024x768 - 4:3)
            case "basement_mini_map":
                worldMinX = -40.0f; worldMaxX = 40.0f;
                worldMinY = -30.0f; worldMaxY = 30.0f;
                break;

            // ID 8: Ancient Sands (1024x768 - 4:3)
            case "ancient_sands_mini_map":
                worldMinX = -46.5f; worldMaxX = 46.5f;
                worldMinY = -35.0f; worldMaxY = 35.0f;
                break;

            // ID 9: Bloodhaven (1207x907 - ~4:3)
            case "bloodhaven_mini_map":
                worldMinX = -46.0f; worldMaxX = 46.0f;
                worldMinY = -34.6f; worldMaxY = 34.6f;
                break;

            // ID 10: Eagleton Springs (1024x1024 - 1:1 Kare)
            case "eagleton_springs_mini_map":
                worldMinX = -42.0f; worldMaxX = 42.0f;
                worldMinY = -42.0f; worldMaxY = 42.0f;
                break;

            // Eagleton Springs Sewers (1024x1024 - 1:1 Kare)
            case "eagleton_springs_sewers_mini_map":
                worldMinX = -32.0f; worldMaxX = 32.0f;
                worldMinY = -32.0f; worldMaxY = 32.0f;
                break;

            // ID 11: The Carnival (1200x900 - 4:3)
            case "carnival_mini_map":
                worldMinX = -50.0f; worldMaxX = 37.0f;
                worldMinY = -62.0f; worldMaxY = 3.2f;
                break;

            // ID 12: Mallardon / Godzilla (1200x900 - 4:3)
            case "mallardon_mini_map":
                worldMinX = -48.0f; worldMaxX = 48.0f;
                worldMinY = -36.0f; worldMaxY = 36.0f;
                break;

            default:
                worldMinX = -45.0f; worldMaxX = 45.0f;
                worldMinY = -45.0f; worldMaxY = 45.0f;
                break;
        }
    }

    public synchronized void updateEntities(List<MapEntity> newEntities) {
        entities.clear();
        if (newEntities != null) {
            entities.addAll(newEntities);
        }
        postInvalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        int w = getWidth();
        int h = getHeight();

        canvas.drawRect(0, 0, w, h, bgPaint);

        Bitmap curBmp = mapBitmap;
        if (curBmp != null && !curBmp.isRecycled()) {
            float bmpW = curBmp.getWidth();
            float bmpH = curBmp.getHeight();
            float scale = Math.min((float) w / bmpW, (float) h / bmpH);
            float drawW = bmpW * scale;
            float drawH = bmpH * scale;
            float left = (w - drawW) * 0.5f;
            float top = (h - drawH) * 0.5f;

            renderMapRect.set(left, top, left + drawW, top + drawH);
            canvas.drawBitmap(curBmp, null, renderMapRect, null);
        } else {
            renderMapRect.set(0, 0, w, h);
            canvas.drawText("Loading: " + currentMapKey, w * 0.5f, h * 0.5f, textPaint);
        }

        synchronized (this) {
            for (MapEntity ent : entities) {
                if (ent.isDead && !showDeadBodies) continue;
                if (!ent.isDead && !ent.isLocal && !showPlayers) continue;

                float normX = (ent.worldX - worldMinX) / (worldMaxX - worldMinX);
                float normY = (worldMaxY - ent.worldY) / (worldMaxY - worldMinY);

                float mapX = renderMapRect.left + (normX * renderMapRect.width());
                float mapY = renderMapRect.top + (normY * renderMapRect.height());

                if (mapX < renderMapRect.left + 4) mapX = renderMapRect.left + 4;
                if (mapX > renderMapRect.right - 4) mapX = renderMapRect.right - 4;
                if (mapY < renderMapRect.top + 4) mapY = renderMapRect.top + 4;
                if (mapY > renderMapRect.bottom - 4) mapY = renderMapRect.bottom - 4;

                if (ent.isDead) {
                    Bitmap icon = deadIcons.get(ent.colorId);
                    if (icon != null) {
                        float iconSize = 22f;
                        RectF dst = new RectF(mapX - iconSize * 0.5f, mapY - iconSize * 0.5f, mapX + iconSize * 0.5f, mapY + iconSize * 0.5f);
                        canvas.drawBitmap(icon, null, dst, bitmapPaint);
                    }
                } else {
                    Bitmap icon = aliveIcons.get(ent.colorId);
                    if (icon != null) {
                        float iconW = 14f;
                        float iconH = 26f;
                        RectF dst = new RectF(mapX - iconW * 0.5f, mapY - iconH * 0.5f, mapX + iconW * 0.5f, mapY + iconH * 0.5f);
                        canvas.drawBitmap(icon, null, dst, bitmapPaint);
                    }
                }

                if (ent.isLocal) {
                    canvas.drawCircle(mapX, mapY, 14f, localRingPaint);
                }
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!touchTeleportEnabled) return super.onTouchEvent(event);

        float touchX = event.getX();
        float touchY = event.getY();

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                handleThrottledTeleport(touchX, touchY, true);
                return true;
            case MotionEvent.ACTION_MOVE:
                handleThrottledTeleport(touchX, touchY, false);
                return true;
            case MotionEvent.ACTION_UP:
                handleThrottledTeleport(touchX, touchY, true);
                return true;
        }
        return super.onTouchEvent(event);
    }

    private void handleThrottledTeleport(float mapX, float mapY, boolean force) {
        long now = System.currentTimeMillis();
        if (!force && (now - lastTeleportTime < 80)) {
            return;
        }
        lastTeleportTime = now;

        if (renderMapRect.width() <= 0 || renderMapRect.height() <= 0) return;

        float normX = Math.max(0f, Math.min(1f, (mapX - renderMapRect.left) / renderMapRect.width()));
        float normY = Math.max(0f, Math.min(1f, (mapY - renderMapRect.top) / renderMapRect.height()));

        float targetWorldX = worldMinX + (normX * (worldMaxX - worldMinX));
        float targetWorldY = worldMaxY - (normY * (worldMaxY - worldMinY));

        try {
            nativeDirectTeleport(targetWorldX, targetWorldY);
        } catch (UnsatisfiedLinkError ignored) {}
    }
}