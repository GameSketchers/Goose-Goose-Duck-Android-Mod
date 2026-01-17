package com.android.support;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Typeface;
import android.os.Build;
import android.view.Choreographer;
import android.view.View;
import android.view.WindowManager;

public class ESPView extends View implements Choreographer.FrameCallback {

    private final Paint linePaint;
    private final Paint boxPaint;
    private final Paint filledBoxPaint;
    private final Paint textPaint;
    private final Paint textBgPaint;
    private final Paint iconPaint;
    private final Paint circlePaint;

    private Typeface iconFont;

    private static final int MAX_LINES = 100;
    private static final int MAX_BOXES = 100;
    private static final int MAX_TEXTS = 100;
    private static final int MAX_ICONS = 50;
    private static final int MAX_CIRCLES = 50;
    private static final int MAX_FILLED_BOXES = 50;

    private final float[][] linesA = new float[MAX_LINES][5];
    private final float[][] boxesA = new float[MAX_BOXES][5];
    private final String[] textStringsA = new String[MAX_TEXTS];
    private final float[][] textDataA = new float[MAX_TEXTS][3];
    private final String[] iconStringsA = new String[MAX_ICONS];
    private final float[][] iconDataA = new float[MAX_ICONS][3];
    private final float[][] circlesA = new float[MAX_CIRCLES][4];
    private final float[][] filledBoxesA = new float[MAX_FILLED_BOXES][5];
    private int lineCountA = 0, boxCountA = 0, textCountA = 0, iconCountA = 0, circleCountA = 0, filledBoxCountA = 0;

    private final float[][] linesB = new float[MAX_LINES][5];
    private final float[][] boxesB = new float[MAX_BOXES][5];
    private final String[] textStringsB = new String[MAX_TEXTS];
    private final float[][] textDataB = new float[MAX_TEXTS][3];
    private final String[] iconStringsB = new String[MAX_ICONS];
    private final float[][] iconDataB = new float[MAX_ICONS][3];
    private final float[][] circlesB = new float[MAX_CIRCLES][4];
    private final float[][] filledBoxesB = new float[MAX_FILLED_BOXES][5];
    private int lineCountB = 0, boxCountB = 0, textCountB = 0, iconCountB = 0, circleCountB = 0, filledBoxCountB = 0;

    private volatile boolean useBufferA = true;
    private volatile boolean isDrawing = false;
    private volatile boolean needsRefresh = false;

    private int realWidth = 0;
    private int realHeight = 0;

    public ESPView(Context context) {
        super(context);

        setLayerType(View.LAYER_TYPE_HARDWARE, null);

        linePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        linePaint.setStrokeWidth(4f);
        linePaint.setStyle(Paint.Style.STROKE);
        linePaint.setStrokeCap(Paint.Cap.ROUND);

        boxPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        boxPaint.setStyle(Paint.Style.STROKE);
        boxPaint.setStrokeWidth(3f);

        filledBoxPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        filledBoxPaint.setStyle(Paint.Style.FILL);

        textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        textPaint.setTextSize(30f);
        textPaint.setStyle(Paint.Style.FILL);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setFakeBoldText(true);

        textBgPaint = new Paint();
        textBgPaint.setColor(Color.argb(180, 0, 0, 0));
        textBgPaint.setStyle(Paint.Style.FILL);

        iconPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        iconPaint.setTextSize(48f);
        iconPaint.setStyle(Paint.Style.FILL);
        iconPaint.setTextAlign(Paint.Align.CENTER);

        circlePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        circlePaint.setStyle(Paint.Style.STROKE);
        circlePaint.setStrokeWidth(3f);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            Choreographer.getInstance().postFrameCallback(this);
        }
    }

    public void setIconFont(Typeface font) {
        this.iconFont = font;
        if (font != null) {
            iconPaint.setTypeface(font);
        }
    }

    @Override
    public void doFrame(long frameTimeNanos) {
        if (isDrawing && needsRefresh) {
            needsRefresh = false;
            invalidate();
        }
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

        boolean readA = !useBufferA;

        float[][] lines = readA ? linesA : linesB;
        float[][] boxes = readA ? boxesA : boxesB;
        float[][] filledBoxes = readA ? filledBoxesA : filledBoxesB;
        float[][] circles = readA ? circlesA : circlesB;
        String[] textStrings = readA ? textStringsA : textStringsB;
        float[][] textData = readA ? textDataA : textDataB;
        String[] iconStrings = readA ? iconStringsA : iconStringsB;
        float[][] iconData = readA ? iconDataA : iconDataB;

        int lc = readA ? lineCountA : lineCountB;
        int bc = readA ? boxCountA : boxCountB;
        int fbc = readA ? filledBoxCountA : filledBoxCountB;
        int cc = readA ? circleCountA : circleCountB;
        int tc = readA ? textCountA : textCountB;
        int ic = readA ? iconCountA : iconCountB;

        for (int i = 0; i < fbc; i++) {
            float[] fb = filledBoxes[i];
            filledBoxPaint.setColor((int) fb[4]);
            canvas.drawRect(fb[0], fb[1], fb[0] + fb[2], fb[1] + fb[3], filledBoxPaint);
        }

        for (int i = 0; i < lc; i++) {
            float[] line = lines[i];
            linePaint.setColor((int) line[4]);
            canvas.drawLine(line[0], line[1], line[2], line[3], linePaint);
        }

        for (int i = 0; i < bc; i++) {
            float[] box = boxes[i];
            boxPaint.setColor((int) box[4]);
            canvas.drawRect(box[0], box[1], box[0] + box[2], box[1] + box[3], boxPaint);
        }

        for (int i = 0; i < cc; i++) {
            float[] circle = circles[i];
            circlePaint.setColor((int) circle[3]);
            canvas.drawCircle(circle[0], circle[1], circle[2], circlePaint);
        }

        for (int i = 0; i < tc; i++) {
            String str = textStrings[i];
            if (str == null || str.isEmpty()) continue;

            float x = textData[i][0];
            float y = textData[i][1];
            int color = (int) textData[i][2];

            float tw = textPaint.measureText(str);
            canvas.drawRect(x - tw / 2 - 5, y - 24, x + tw / 2 + 5, y + 6, textBgPaint);

            textPaint.setColor(color);
            canvas.drawText(str, x, y, textPaint);
        }

        if (iconFont != null) {
            for (int i = 0; i < ic; i++) {
                String iconStr = iconStrings[i];
                if (iconStr == null || iconStr.isEmpty()) continue;

                float x = iconData[i][0];
                float y = iconData[i][1];
                int color = (int) iconData[i][2];

                iconPaint.setColor(color);
                canvas.drawText(iconStr, x, y, iconPaint);
            }
        }
    }

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

    public void addFilledBox(float x, float y, float w, float h, int color) {
        if (useBufferA) {
            if (filledBoxCountA < MAX_FILLED_BOXES) {
                filledBoxesA[filledBoxCountA][0] = x;
                filledBoxesA[filledBoxCountA][1] = y;
                filledBoxesA[filledBoxCountA][2] = w;
                filledBoxesA[filledBoxCountA][3] = h;
                filledBoxesA[filledBoxCountA][4] = color;
                filledBoxCountA++;
            }
        } else {
            if (filledBoxCountB < MAX_FILLED_BOXES) {
                filledBoxesB[filledBoxCountB][0] = x;
                filledBoxesB[filledBoxCountB][1] = y;
                filledBoxesB[filledBoxCountB][2] = w;
                filledBoxesB[filledBoxCountB][3] = h;
                filledBoxesB[filledBoxCountB][4] = color;
                filledBoxCountB++;
            }
        }
    }

    public void addCircle(float x, float y, float radius, int color) {
        if (useBufferA) {
            if (circleCountA < MAX_CIRCLES) {
                circlesA[circleCountA][0] = x;
                circlesA[circleCountA][1] = y;
                circlesA[circleCountA][2] = radius;
                circlesA[circleCountA][3] = color;
                circleCountA++;
            }
        } else {
            if (circleCountB < MAX_CIRCLES) {
                circlesB[circleCountB][0] = x;
                circlesB[circleCountB][1] = y;
                circlesB[circleCountB][2] = radius;
                circlesB[circleCountB][3] = color;
                circleCountB++;
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

    public void addIcon(float x, float y, String icon, int color) {
        if (icon == null || iconFont == null) return;
        if (useBufferA) {
            if (iconCountA < MAX_ICONS) {
                iconStringsA[iconCountA] = icon;
                iconDataA[iconCountA][0] = x;
                iconDataA[iconCountA][1] = y;
                iconDataA[iconCountA][2] = color;
                iconCountA++;
            }
        } else {
            if (iconCountB < MAX_ICONS) {
                iconStringsB[iconCountB] = icon;
                iconDataB[iconCountB][0] = x;
                iconDataB[iconCountB][1] = y;
                iconDataB[iconCountB][2] = color;
                iconCountB++;
            }
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

    public void addIcon(float x, float y, String icon) {
        addIcon(x, y, icon, Color.WHITE);
    }

    public void addCircle(float x, float y, float radius) {
        addCircle(x, y, radius, Color.YELLOW);
    }

    public void addFilledBox(float x, float y, float w, float h) {
        addFilledBox(x, y, w, h, Color.argb(100, 0, 0, 0));
    }

    public void clearAll() {
        if (useBufferA) {
            lineCountA = 0;
            boxCountA = 0;
            textCountA = 0;
            iconCountA = 0;
            circleCountA = 0;
            filledBoxCountA = 0;
        } else {
            lineCountB = 0;
            boxCountB = 0;
            textCountB = 0;
            iconCountB = 0;
            circleCountB = 0;
            filledBoxCountB = 0;
        }
    }

    public void refresh() {
        useBufferA = !useBufferA;
        needsRefresh = true;
    }

    public void setDrawing(boolean drawing) {
        isDrawing = drawing;
        if (!drawing) {
            lineCountA = lineCountB = 0;
            boxCountA = boxCountB = 0;
            textCountA = textCountB = 0;
            iconCountA = iconCountB = 0;
            circleCountA = circleCountB = 0;
            filledBoxCountA = filledBoxCountB = 0;
            postInvalidate();
        }
    }

    public boolean isDrawingEnabled() {
        return isDrawing;
    }
}
