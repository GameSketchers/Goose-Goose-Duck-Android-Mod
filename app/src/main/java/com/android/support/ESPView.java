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

public class ESPView extends View implements Choreographer.FrameCallback {

    // Paint objects
    private final Paint linePaint;
    private final Paint boxPaint;
    private final Paint textPaint;
    private final Paint textBgPaint;

    // Double buffer - A ve B
    private static final int MAX_LINES = 100;
    private static final int MAX_BOXES = 100;
    private static final int MAX_TEXTS = 100;

    // Buffer A
    private final float[][] linesA = new float[MAX_LINES][5];
    private final float[][] boxesA = new float[MAX_BOXES][5];
    private final String[] textStringsA = new String[MAX_TEXTS];
    private final float[][] textDataA = new float[MAX_TEXTS][3]; // x, y, color
    private int lineCountA = 0, boxCountA = 0, textCountA = 0;

    // Buffer B
    private final float[][] linesB = new float[MAX_LINES][5];
    private final float[][] boxesB = new float[MAX_BOXES][5];
    private final String[] textStringsB = new String[MAX_TEXTS];
    private final float[][] textDataB = new float[MAX_TEXTS][3];
    private int lineCountB = 0, boxCountB = 0, textCountB = 0;

    // State
    private volatile boolean useBufferA = true;
    private volatile boolean isDrawing = false;
    private volatile boolean needsRefresh = false;

    private int realWidth = 0;
    private int realHeight = 0;

    public ESPView(Context context) {
        super(context);

        // Hardware acceleration
        setLayerType(View.LAYER_TYPE_HARDWARE, null);

        // Line paint
        linePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        linePaint.setStrokeWidth(4f);
        linePaint.setStyle(Paint.Style.STROKE);
        linePaint.setStrokeCap(Paint.Cap.ROUND);

        // Box paint
        boxPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        boxPaint.setStyle(Paint.Style.STROKE);
        boxPaint.setStrokeWidth(3f);

        // Text paint
        textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        textPaint.setTextSize(30f);
        textPaint.setStyle(Paint.Style.FILL);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setFakeBoldText(true);

        // Text background paint
        textBgPaint = new Paint();
        textBgPaint.setColor(Color.argb(180, 0, 0, 0));
        textBgPaint.setStyle(Paint.Style.FILL);

        // Start choreographer
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            Choreographer.getInstance().postFrameCallback(this);
        }
    }

    @Override
    public void doFrame(long frameTimeNanos) {
        if (isDrawing && needsRefresh) {
            needsRefresh = false;
            invalidate();
        }
        // Re-register for next frame
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
			WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
			PixelFormat.TRANSLUCENT
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            params.layoutInDisplayCutoutMode =
				WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
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
        if (!isDrawing) return;

        // Okuma buffer'ını belirle (yazma buffer'ının tersi)
        boolean readA = !useBufferA;

        float[][] lines = readA ? linesA : linesB;
        float[][] boxes = readA ? boxesA : boxesB;
        String[] textStrings = readA ? textStringsA : textStringsB;
        float[][] textData = readA ? textDataA : textDataB;
        int lc = readA ? lineCountA : lineCountB;
        int bc = readA ? boxCountA : boxCountB;
        int tc = readA ? textCountA : textCountB;

        // Draw lines
        for (int i = 0; i < lc; i++) {
            float[] line = lines[i];
            linePaint.setColor((int) line[4]);
            canvas.drawLine(line[0], line[1], line[2], line[3], linePaint);
        }

        // Draw boxes
        for (int i = 0; i < bc; i++) {
            float[] box = boxes[i];
            boxPaint.setColor((int) box[4]);
            canvas.drawRect(box[0], box[1], box[0] + box[2], box[1] + box[3], boxPaint);
        }

        // Draw texts
        for (int i = 0; i < tc; i++) {
            String str = textStrings[i];
            if (str == null || str.isEmpty()) continue;

            float x = textData[i][0];
            float y = textData[i][1];
            int color = (int) textData[i][2];

            // Background
            float tw = textPaint.measureText(str);
            canvas.drawRect(x - tw / 2 - 5, y - 24, x + tw / 2 + 5, y + 6, textBgPaint);

            // Text
            textPaint.setColor(color);
            canvas.drawText(str, x, y, textPaint);
        }
    }

    // ==================== ADD METHODS (Write to active buffer) ====================

    public void addLine(float x1, float y1, float x2, float y2, int color) {
        if (useBufferA) {
            if (lineCountA < MAX_LINES) {
                linesA[lineCountA][0] = x1;
                linesA[lineCountA][1] = y1;
                linesA[lineCountA][2] = x2;
                linesA[lineCountA][3] = y2;
                linesA[lineCountA][4] = color;
                lineCountA++;
            }
        } else {
            if (lineCountB < MAX_LINES) {
                linesB[lineCountB][0] = x1;
                linesB[lineCountB][1] = y1;
                linesB[lineCountB][2] = x2;
                linesB[lineCountB][3] = y2;
                linesB[lineCountB][4] = color;
                lineCountB++;
            }
        }
    }

    public void addBox(float x, float y, float w, float h, int color) {
        if (useBufferA) {
            if (boxCountA < MAX_BOXES) {
                boxesA[boxCountA][0] = x;
                boxesA[boxCountA][1] = y;
                boxesA[boxCountA][2] = w;
                boxesA[boxCountA][3] = h;
                boxesA[boxCountA][4] = color;
                boxCountA++;
            }
        } else {
            if (boxCountB < MAX_BOXES) {
                boxesB[boxCountB][0] = x;
                boxesB[boxCountB][1] = y;
                boxesB[boxCountB][2] = w;
                boxesB[boxCountB][3] = h;
                boxesB[boxCountB][4] = color;
                boxCountB++;
            }
        }
    }

    public void addText(float x, float y, String text, int color) {
        if (text == null) return;
        if (useBufferA) {
            if (textCountA < MAX_TEXTS) {
                textStringsA[textCountA] = text;
                textDataA[textCountA][0] = x;
                textDataA[textCountA][1] = y;
                textDataA[textCountA][2] = color;
                textCountA++;
            }
        } else {
            if (textCountB < MAX_TEXTS) {
                textStringsB[textCountB] = text;
                textDataB[textCountB][0] = x;
                textDataB[textCountB][1] = y;
                textDataB[textCountB][2] = color;
                textCountB++;
            }
        }
    }

    // Default color overloads
    public void addLine(float x1, float y1, float x2, float y2) {
        addLine(x1, y1, x2, y2, Color.RED);
    }

    public void addBox(float x, float y, float w, float h) {
        addBox(x, y, w, h, Color.GREEN);
    }

    public void addText(float x, float y, String text) {
        addText(x, y, text, Color.WHITE);
    }

    // ==================== CONTROL METHODS ====================

    public void clearAll() {
        // Aktif yazma buffer'ını temizle
        if (useBufferA) {
            lineCountA = 0;
            boxCountA = 0;
            textCountA = 0;
        } else {
            lineCountB = 0;
            boxCountB = 0;
            textCountB = 0;
        }
    }

    public void refresh() {
        // Buffer swap - atomik
        useBufferA = !useBufferA;
        needsRefresh = true;
    }

    public void setDrawing(boolean drawing) {
        isDrawing = drawing;
        if (!drawing) {
            // Her iki buffer'ı da temizle
            lineCountA = lineCountB = 0;
            boxCountA = boxCountB = 0;
            textCountA = textCountB = 0;
            postInvalidate();
        }
    }

    public boolean isDrawingEnabled() {
        return isDrawing;
    }
}
