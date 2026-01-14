package com.android.support;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.widget.Toast;

import java.io.File;
import java.io.InputStream;

public class Main {

    static {
        System.loadLibrary("anonimbiri");
    }

    private static final String OBB_FILE_NAME = "main.200524.com.Gaggle.fun.GooseGooseDuck.obb";

    private static native void CheckOverlayPermission(Context context);

    public static void Start(final Context context) {
        CrashHandler.init(context, false);

        // OBB kontrolü
        if (!isObbInstalled(context) && isObbInAssets(context)) {
            // OBB kurulumu gerekli - Activity aç
            Intent intent = new Intent(context, ObbInstallerActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
            // Activity bitince tekrar Start çağrılacak
            return;
        }

        // OBB tamam, orijinal akışa devam
        CheckOverlayPermission(context);
    }

    public static void StartWithoutPermission(Context context) {
        CrashHandler.init(context, true);
        if (context instanceof Activity) {
            Menu menu = new Menu(context);
            menu.SetWindowManagerActivity();
            menu.ShowMenu();
        } else {
            Toast.makeText(context, "Failed to launch the mod menu", Toast.LENGTH_LONG).show();
        }
    }

    private static boolean isObbInstalled(Context context) {
        try {
            File obbDir = context.getObbDir();
            if (!obbDir.exists()) return false;

            // Belirli dosya
            File obbFile = new File(obbDir, OBB_FILE_NAME);
            if (obbFile.exists() && obbFile.length() > 1000) {
                return true;
            }

            // Herhangi bir .obb
            File[] files = obbDir.listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.getName().endsWith(".obb") && file.length() > 1000) {
                        return true;
                    }
                }
            }
            return false;
        } catch (Exception e) {
            return false;
        }
    }

    private static boolean isObbInAssets(Context context) {
        try {
            InputStream is = context.getAssets().open(OBB_FILE_NAME);
            is.close();
            return true;
        } catch (Exception e) {
            return false;
        }
    }
}
