package su.xash.engine;

import android.annotation.SuppressLint;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.content.SharedPreferences;
import android.provider.Settings.Secure;
import android.util.Log;
import android.view.KeyEvent;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

import su.xash.engine.util.AndroidBug5497Workaround;

import java.io.File;
import java.util.Arrays;
import java.util.List;

public class XashActivity extends SDLActivity {
    private boolean mUseVolumeKeys;
    private String mPackageName;
    private static final String TAG = "XashActivity";
    private SharedPreferences mPreferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mPreferences = PreferenceManager.getDefaultSharedPreferences(this);

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        AndroidBug5497Workaround.assistActivity(this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        System.exit(0);
    }

    @Override
    protected String[] getLibraries() {
        return new String[]{"SDL2", "xash"};
    }

    @SuppressLint("HardwareIds")
    private String getAndroidID() {
        return Secure.getString(getContentResolver(), Secure.ANDROID_ID);
    }

    @SuppressLint("ApplySharedPref")
    private void saveAndroidID(String id) {
        getSharedPreferences("xash_preferences", MODE_PRIVATE).edit().putString("xash_id", id).commit();
    }

    private String loadAndroidID() {
        return getSharedPreferences("xash_preferences", MODE_PRIVATE).getString("xash_id", "");
    }

    @Override
    public String getCallingPackage() {
        if (mPackageName != null) {
            return mPackageName;
        }
        return super.getCallingPackage();
    }

    private AssetManager getAssets(boolean isEngine) {
        AssetManager am = null;
        if (isEngine) {
            am = getAssets();
        } else {
            try {
                am = getPackageManager().getResourcesForApplication(getCallingPackage()).getAssets();
            } catch (Exception e) {
                Log.e(TAG, "Unable to load mod assets!");
                e.printStackTrace();
            }
        }
        return am;
    }

    private String[] getAssetsList(boolean isEngine, String path) {
        AssetManager am = getAssets(isEngine);
        try {
            return am.list(path);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return new String[]{};
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (SDLActivity.mBrokenLibraries) {
            return false;
        }
        int keyCode = event.getKeyCode();
        if (!mUseVolumeKeys) {
            if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP || keyCode == KeyEvent.KEYCODE_CAMERA || keyCode == KeyEvent.KEYCODE_ZOOM_IN || keyCode == KeyEvent.KEYCODE_ZOOM_OUT) {
                return false;
            }
        }
        return getWindow().superDispatchKeyEvent(event);
    }

    private void debugIntentParameters() {
        Log.d(TAG, "=== INTENT PARAMETERS ===");
        Log.d(TAG, "gamedir: " + getIntent().getStringExtra("gamedir"));
        Log.d(TAG, "gamelibdir: " + getIntent().getStringExtra("gamelibdir"));
        Log.d(TAG, "basedir: " + getIntent().getStringExtra("basedir"));
        Log.d(TAG, "pakfile: " + getIntent().getStringExtra("pakfile"));
        Log.d(TAG, "argv: " + getIntent().getStringExtra("argv"));
        
        String[] env = getIntent().getStringArrayExtra("env");
        if (env != null) {
            for (int i = 0; i < env.length; i += 2) {
                Log.d(TAG, "env[" + env[i] + "] = " + env[i + 1]);
            }
        }
        Log.d(TAG, "=== END PARAMETERS ===");
    }

    private String findGameDirectory(String gameDir) {
        File externalDir = new File(Environment.getExternalStorageDirectory().getAbsolutePath() + "/xash/" + gameDir);
        if (externalDir.exists() && externalDir.isDirectory()) {
            Log.d(TAG, "Found game directory in external storage: " + externalDir.getAbsolutePath());
            fixGameDirectoryPermissions(externalDir.getAbsolutePath());
            return Environment.getExternalStorageDirectory().getAbsolutePath() + "/xash";
        }
        
        File internalDir = new File(getExternalFilesDir(null).getAbsolutePath() + "/" + gameDir);
        if (internalDir.exists() && internalDir.isDirectory()) {
            Log.d(TAG, "Found game directory in internal storage: " + internalDir.getAbsolutePath());
            fixGameDirectoryPermissions(internalDir.getAbsolutePath());
            return getExternalFilesDir(null).getAbsolutePath();
        }
        
        Log.d(TAG, "Game directory not found, using external storage as default");
        return Environment.getExternalStorageDirectory().getAbsolutePath() + "/xash";
    }

    private void fixGameDirectoryPermissions(String gameDirPath) {
        try {
            File gameDirectory = new File(gameDirPath);
            if (gameDirectory.exists() && gameDirectory.isDirectory()) {
                gameDirectory.setReadable(true, false);
                gameDirectory.setWritable(true, false);
                gameDirectory.setExecutable(true, false);
                
                File[] files = gameDirectory.listFiles();
                if (files != null) {
                    for (File file : files) {
                        file.setReadable(true, false);
                        file.setWritable(true, false);
                        file.setExecutable(true, false);
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Error fixing permissions for: " + gameDirPath, e);
        }
    }

    @Override
    protected String[] getArguments() {
        debugIntentParameters();
        
        String gamedir = getIntent().getStringExtra("gamedir");
        if (gamedir == null) gamedir = "valve";
        
        String basedir = findGameDirectory(gamedir);
        
        // nativeSetenv("XASH3D_BASEDIR", basedir);
        
        Log.d(TAG, "Final settings - gamedir: " + gamedir + ", basedir: " + basedir);

        String gamelibdir = getIntent().getStringExtra("gamelibdir");
        if (gamelibdir != null) nativeSetenv("XASH3D_GAMELIBDIR", gamelibdir);

        String pakfile = getIntent().getStringExtra("pakfile");
        if (pakfile != null) nativeSetenv("XASH3D_EXTRAS_PAK2", pakfile);

        mUseVolumeKeys = getIntent().getBooleanExtra("usevolume", false);
        mPackageName = getIntent().getStringExtra("package");

        String[] env = getIntent().getStringArrayExtra("env");
        if (env != null) {
            for (int i = 0; i < env.length; i += 2)
                nativeSetenv(env[i], env[i + 1]);
        }

        String argv = getIntent().getStringExtra("argv");
        if (argv == null) argv = "-console -log";
        
        if (argv.contains("-game") && !argv.contains("-game valve")) {
            Log.d(TAG, "Found -game parameter in argv: " + argv);
        }

        nativeSetenv("XASH3D_GAME", gamedir);
        
        if (!argv.contains("-game") && !gamedir.equals("valve")) {
            argv += " -game " + gamedir;
            Log.d(TAG, "Added -game parameter to argv: " + argv);
        }

        if (argv.indexOf(" -dll ") < 0 && gamelibdir == null) {
            final List<String> mobile_hacks_gamedirs = Arrays.asList(new String[]{
                "aom", "bdlands", "biglolly", "bshift", "caseclosed",
                "hl_urbicide", "induction", "redempt", "secret",
                "sewer_beta", "tot", "vendetta" });

            if (mobile_hacks_gamedirs.contains(gamedir))
                argv += " -dll @hl";
        }

        Log.d(TAG, "Final argv: " + argv);
        return argv.split(" ");
    }
}
