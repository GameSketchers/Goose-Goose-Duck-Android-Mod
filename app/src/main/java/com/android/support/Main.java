package com.android.support;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.widget.Toast;

import java.io.File;
import java.io.InputStream;

public class Main {

    static {
        System.loadLibrary("anonimbiri");
    }

    private static native void CheckOverlayPermission(Context context);

    public static void Start(final Context context) {
        CrashHandler.init(context, false);

        if (!isObbInstalled(context) && isObbInAssets(context)) {
            Intent intent = new Intent(context, ObbInstallerActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
            return;
        }

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

    private static String getObbFileName(Context context) {
        try {
            PackageManager pm = context.getPackageManager();
            PackageInfo pInfo = pm.getPackageInfo(context.getPackageName(), 0);
            long versionCode;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                versionCode = pInfo.getLongVersionCode();
            } else {
                versionCode = pInfo.versionCode;
            }
            return "main." + versionCode + "." + context.getPackageName() + ".obb";
        } catch (Exception e) {
            return "main.0." + context.getPackageName() + ".obb";
        }
    }

    private static boolean isObbInstalled(Context context) {
        try {
            File obbDir = context.getObbDir();
            if (!obbDir.exists()) return false;

            String obbFileName = getObbFileName(context);
            File obbFile = new File(obbDir, obbFileName);
            if (obbFile.exists() && obbFile.length() > 1000) {
                return true;
            }

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
            String obbFileName = getObbFileName(context);
            InputStream is = context.getAssets().open(obbFileName);
            is.close();
            return true;
        } catch (Exception e) {
            return false;
        }
    }
}