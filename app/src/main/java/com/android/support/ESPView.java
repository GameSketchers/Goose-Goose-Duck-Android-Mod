package com.android.support;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.os.Build;
import android.view.Choreographer;
import android.view.View;
import android.view.WindowManager;

import java.util.ArrayList;
import java.util.List;

public class ESPView extends View implements Choreographer.FrameCallback {

    private final Paint linePaint;
    private final Paint boxPaint;
    private final Paint textPaint;
    private final Paint textBgPaint;

    private final List<float[]> lines = new ArrayList<>();
    private final List<float[]> boxes = new ArrayList<>();
    private final List<Object[]> texts = new ArrayList<>();

    private volatile boolean isDrawing = false;
    private int realWidth = 0;
    private int realHeight = 0;

    private boolean useChoreographer = true;

    public ESPView(Context context) {
        super(context);

        setLayerType(View.LAYER_TYPE_HARDWARE, null);

        linePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        linePaint.setStrokeWidth(5);
        linePaint.setStyle(Paint.Style.STROKE);

        boxPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        boxPaint.setStyle(Paint.Style.STROKE);
        boxPaint.setStrokeWidth(4);

        textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        textPaint.setTextSize(36);
        textPaint.setStyle(Paint.Style.FILL);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setFakeBoldText(true);

        textBgPaint = new Paint();
        textBgPaint.setColor(Color.argb(200, 0, 0, 0));
        textBgPaint.setStyle(Paint.Style.FILL);

        // Choreographer başlat
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            Choreographer.getInstance().postFrameCallback(this);
        }
    }

    @Override
    public void doFrame(long frameTimeNanos) {
        if (isDrawing && useChoreographer) {
            invalidate();
        }
        // Sonraki frame için tekrar kayıt ol
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            Choreographer.getInstance().postFrameCallback(this);
        }
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
            WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS |
            WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
            PixelFormat.TRANSLUCENT
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            params.layoutInDisplayCutoutMode = 
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        return params;
    }

    public int getRealWidth() { return realWidth > 0 ? realWidth : 1080; }
    public int getRealHeight() { return realHeight > 0 ? realHeight : 2400; }

    @Override
    protected void onDraw(Canvas canvas) {
        if (!isDrawing) return;

        if (realWidth == 0 || realHeight == 0) {
            realWidth = canvas.getWidth();
            realHeight = canvas.getHeight();
        }

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
                    canvas.drawRect(x - tw/2 - 6, y - 28, x + tw/2 + 6, y + 6, textBgPaint);
                    textPaint.setColor(color);
                    canvas.drawText(str, x, y, textPaint);
                }
            }
        }
    }

    public void addLine(float x1, float y1, float x2, float y2, int color) {
        synchronized (lines) { lines.add(new float[]{x1, y1, x2, y2, color}); }
    }

    public void addBox(float x, float y, float w, float h, int color) {
        synchronized (boxes) { boxes.add(new float[]{x, y, w, h, color}); }
    }

    public void addText(float x, float y, String text, int color) {
        if (text == null) return;
        synchronized (texts) { texts.add(new Object[]{x, y, text, color}); }
    }

    public void addLine(float x1, float y1, float x2, float y2) { addLine(x1, y1, x2, y2, Color.RED); }
    public void addBox(float x, float y, float w, float h) { addBox(x, y, w, h, Color.GREEN); }
    public void addText(float x, float y, String text) { addText(x, y, text, Color.WHITE); }

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
        // Choreographer kullanıyoruz, ek invalidate gerekli değil
        // Ama manuel çağrı için:
        if (!useChoreographer) {
            postInvalidateOnAnimation();
        }
    }

    public void stopChoreographer() {
        useChoreographer = false;
    }
}
