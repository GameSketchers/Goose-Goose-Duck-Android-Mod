package com.android.support;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.os.Build;
import android.view.View;
import android.view.WindowManager;

import java.util.ArrayList;
import java.util.List;

public class ESPView extends View {

    private Paint linePaint;
    private Paint boxPaint;
    private Paint textPaint;
    private Paint textBgPaint;

    private List<float[]> lines;
    private List<float[]> boxes;
    private List<Object[]> texts;

    private boolean isDrawing;

    private int realWidth = 0;
    private int realHeight = 0;

    public ESPView(Context context) {
        super(context);
        lines = new ArrayList<>();
        boxes = new ArrayList<>();
        texts = new ArrayList<>();
        isDrawing = false;
        init();
    }

    private void init() {
        // Hardware acceleration for better performance
        setLayerType(View.LAYER_TYPE_HARDWARE, null);

        linePaint = new Paint();
        linePaint.setStrokeWidth(5);  // Biraz daha kalın
        linePaint.setAntiAlias(true);
        linePaint.setStyle(Paint.Style.STROKE);

        boxPaint = new Paint();
        boxPaint.setStyle(Paint.Style.STROKE);
        boxPaint.setStrokeWidth(4);
        boxPaint.setAntiAlias(true);

        textPaint = new Paint();
        textPaint.setTextSize(34);  // BÜYÜTÜLDÜ: 26 -> 34
        textPaint.setStyle(Paint.Style.FILL);
        textPaint.setAntiAlias(true);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setFakeBoldText(true);  // Kalın yazı

        textBgPaint = new Paint();
        textBgPaint.setColor(Color.argb(200, 0, 0, 0));  // Daha koyu arka plan
        textBgPaint.setStyle(Paint.Style.FILL);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        realWidth = w;
        realHeight = h;
    }

    public static WindowManager.LayoutParams createLayoutParams() {
        int layoutType;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            layoutType = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        } else {
            layoutType = WindowManager.LayoutParams.TYPE_PHONE;
        }

        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.MATCH_PARENT,
            layoutType,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
            WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE |
            WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN |
            WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
            PixelFormat.TRANSLUCENT
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        return params;
    }

    public int getRealWidth() {
        return realWidth > 0 ? realWidth : 1080;
    }

    public int getRealHeight() {
        return realHeight > 0 ? realHeight : 2400;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (!isDrawing) return;

        if (realWidth == 0 || realHeight == 0) {
            realWidth = canvas.getWidth();
            realHeight = canvas.getHeight();
        }

        try {
            // Lines
            synchronized (lines) {
                for (float[] line : lines) {
                    if (line != null && line.length >= 5) {
                        linePaint.setColor((int) line[4]);
                        canvas.drawLine(line[0], line[1], line[2], line[3], linePaint);
                    }
                }
            }

            // Boxes
            synchronized (boxes) {
                for (float[] box : boxes) {
                    if (box != null && box.length >= 5) {
                        boxPaint.setColor((int) box[4]);
                        canvas.drawRect(box[0], box[1], box[0] + box[2], box[1] + box[3], boxPaint);
                    }
                }
            }

            // Texts
            synchronized (texts) {
                for (Object[] text : texts) {
                    if (text != null && text.length >= 4 && text[2] != null) {
                        float x = ((Number) text[0]).floatValue();
                        float y = ((Number) text[1]).floatValue();
                        String str = (String) text[2];
                        int color = ((Number) text[3]).intValue();

                        float tw = textPaint.measureText(str);
                        float padding = 8;

                        // Arka plan
                        canvas.drawRect(
                            x - tw/2 - padding, 
                            y - 28,  // Daha yüksek
                            x + tw/2 + padding, 
                            y + 8, 
                            textBgPaint
                        );

                        // Text
                        textPaint.setColor(color);
                        canvas.drawText(str, x, y, textPaint);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void addLine(float x1, float y1, float x2, float y2, int color) {
        synchronized (lines) {
            lines.add(new float[]{x1, y1, x2, y2, color});
        }
    }

    public void addBox(float x, float y, float w, float h, int color) {
        synchronized (boxes) {
            boxes.add(new float[]{x, y, w, h, color});
        }
    }

    public void addText(float x, float y, String text, int color) {
        if (text == null) return;
        synchronized (texts) {
            texts.add(new Object[]{x, y, text, color});
        }
    }

    public void addLine(float x1, float y1, float x2, float y2) {
        addLine(x1, y1, x2, y2, Color.RED);
    }

    public void addBox(float x, float y, float w, float h) {
        addBox(x, y, w, h, Color.GREEN);
    }

    public void addText(float x, float y, String text) {
        addText(x, y, text, Color.WHITE);
    }

    public void clearAll() {
        synchronized (lines) { lines.clear(); }
        synchronized (boxes) { boxes.clear(); }
        synchronized (texts) { texts.clear(); }
    }

    public void setDrawing(boolean d) {
        isDrawing = d;
        if (!d) clearAll();
    }

    public void refresh() {
        // Ana thread'de çağır - daha hızlı güncelleme
        post(new Runnable() {
				@Override
				public void run() {
					invalidate();
				}
			});
    }
}
