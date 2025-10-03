package su.xash.engine;

import android.annotation.SuppressLint;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.content.SharedPreferences;
import android.provider.Settings.Secure;
import android.util.Log;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;
import android.view.View;

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
    private int mCustomWidth;
    private int mCustomHeight;
    private float mResolutionScale;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mPreferences = getSharedPreferences("app_preferences", MODE_PRIVATE);

        mCustomWidth = Integer.parseInt(mPreferences.getString("resolution_width", "0"));
        mCustomHeight = Integer.parseInt(mPreferences.getString("resolution_height", "0"));
        mResolutionScale = Float.parseFloat(mPreferences.getString("resolution_scale", "1.0"));

        overrideDisplayMetrics();

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        AndroidBug5497Workaround.assistActivity(this);
    }

    private void overrideDisplayMetrics() {
        DisplayMetrics realMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(realMetrics);
        
        int targetWidth;
        int targetHeight;
        
        if (mCustomWidth > 0 && mCustomHeight > 0) {
            targetWidth = mCustomWidth;
            targetHeight = mCustomHeight;
            Log.d(TAG, "Overriding display to: " + targetWidth + "x" + targetHeight);
        } else if (mResolutionScale != 1.0f) {
            targetWidth = (int)(realMetrics.widthPixels / mResolutionScale);
            targetHeight = (int)(realMetrics.heightPixels / mResolutionScale);
            Log.d(TAG, "Overriding display with scale " + mResolutionScale + ": " + targetWidth + "x" + targetHeight);
        } else {
            Log.d(TAG, "Using native resolution: " + realMetrics.widthPixels + "x" + realMetrics.heightPixels);
            return;
        }

        DisplayMetrics fakeMetrics = new DisplayMetrics();
        fakeMetrics.widthPixels = targetWidth;
        fakeMetrics.heightPixels = targetHeight;
        fakeMetrics.density = realMetrics.density;
        fakeMetrics.densityDpi = realMetrics.densityDpi;
        fakeMetrics.scaledDensity = realMetrics.scaledDensity;
        fakeMetrics.xdpi = realMetrics.xdpi;
        fakeMetrics.ydpi = realMetrics.ydpi;

        overrideResourcesMetrics(fakeMetrics);
        
        notifySDLAboutResolution(targetWidth, targetHeight, realMetrics.widthPixels, realMetrics.heightPixels);
    }

    private void overrideResourcesMetrics(DisplayMetrics metrics) {
        try {
            Resources resources = getResources();
            resources.getDisplayMetrics().setTo(metrics);

            Resources activityResources = super.getResources();
            activityResources.getDisplayMetrics().setTo(metrics);

            Log.d(TAG, "Resources metrics overridden to: " + metrics.widthPixels + "x" + metrics.heightPixels);
            
        } catch (Exception e) {
            Log.e(TAG, "Failed to override resources metrics: " + e.getMessage());
        }
    }

    private void notifySDLAboutResolution(int width, int height, int deviceWidth, int deviceHeight) {
        try {
            float refreshRate = getWindowManager().getDefaultDisplay().getRefreshRate();
            
            SDLActivity.nativeSetScreenResolution(width, height, deviceWidth, deviceHeight, refreshRate);
            
            nativeSetenv("SDL_VIDEO_WINDOW_WIDTH", String.valueOf(width));
            nativeSetenv("SDL_VIDEO_WINDOW_HEIGHT", String.valueOf(height));
            nativeSetenv("XASH_RESOLUTION_WIDTH", String.valueOf(width));
            nativeSetenv("XASH_RESOLUTION_HEIGHT", String.valueOf(height));
            
            Log.d(TAG, "Notified SDL about resolution: " + width + "x" + height);
            
        } catch (Exception e) {
            Log.e(TAG, "Error notifying SDL: " + e.getMessage());
        }
    }

    @Override
    public Resources getResources() {
        Resources res = super.getResources();
        if ((mCustomWidth > 0 || mResolutionScale != 1.0f)) {
            DisplayMetrics metrics = res.getDisplayMetrics();
            
            if ((mCustomWidth > 0 && metrics.widthPixels != mCustomWidth) || 
                (mResolutionScale != 1.0f && metrics.widthPixels != (int)(getRealWidth() / mResolutionScale))) {
                overrideDisplayMetrics();
            }
        }
        return res;
    }

    private int getRealWidth() {
        DisplayMetrics realMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(realMetrics);
        return realMetrics.widthPixels;
    }

    private int getRealHeight() {
        DisplayMetrics realMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(realMetrics);
        return realMetrics.heightPixels;
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        
        if (hasFocus) {
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    overrideDisplayMetrics();
                }
            }, 100);
        }
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

    private String getGlobalArguments() {
        String globalArgs = mPreferences.getString("global_arguments", "");
        if (globalArgs != null && !globalArgs.trim().isEmpty()) {
            return globalArgs.trim();
        }
        return "";
    }

    private String combineArguments(String originalArgs, String globalArgs) {
        if (globalArgs.isEmpty()) {
            return originalArgs;
        }
        
        if (originalArgs == null || originalArgs.trim().isEmpty()) {
            return globalArgs;
        }
        
        return originalArgs.trim() + " " + globalArgs;
    }

    private String findBestBasedir(String gamedir) {
        File internalDir = new File(getExternalFilesDir(null).getAbsolutePath() + "/" + gamedir);
        if (internalDir.exists() && internalDir.isDirectory()) {
            Log.d(TAG, "Game found in internal storage: " + internalDir.getAbsolutePath());
            return getExternalFilesDir(null).getAbsolutePath();
        }
        
        File externalDir = new File(Environment.getExternalStorageDirectory().getAbsolutePath() + "/xash/" + gamedir);
        if (externalDir.exists() && externalDir.isDirectory()) {
            Log.d(TAG, "Game found in external storage: " + externalDir.getAbsolutePath());
            return Environment.getExternalStorageDirectory().getAbsolutePath() + "/xash";
        }
        
        boolean useInternalStorage = mPreferences.getBoolean("storage_toggle", false);
        if (useInternalStorage) {
            Log.d(TAG, "Game not found, using internal storage as default");
            return getExternalFilesDir(null).getAbsolutePath();
        } else {
            Log.d(TAG, "Game not found, using external storage as default");
            return Environment.getExternalStorageDirectory().getAbsolutePath() + "/xash";
        }
    }

    @Override
    protected String[] getArguments() {
        String gamedir = getIntent().getStringExtra("gamedir");
        if (gamedir == null) gamedir = "valve";
        
        String basedir = findBestBasedir(gamedir);
        nativeSetenv("XASH3D_BASEDIR", basedir);
        nativeSetenv("XASH3D_GAME", gamedir);
        
        Log.d(TAG, "Using basedir: " + basedir + " for game: " + gamedir);

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

        String globalArgs = getGlobalArguments();
        if (!globalArgs.isEmpty()) {
            Log.d(TAG, "Global arguments found: " + globalArgs);
            argv = combineArguments(argv, globalArgs);
        }

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

        applyFinalResolutionSettings();

        Log.d(TAG, "Final argv: " + argv);
        return argv.split(" ");
    }

    private void applyFinalResolutionSettings() {
        if (mCustomWidth > 0 && mCustomHeight > 0) {
            nativeSetenv("SDL_VIDEO_FORCE_WIDTH", String.valueOf(mCustomWidth));
            nativeSetenv("SDL_VIDEO_FORCE_HEIGHT", String.valueOf(mCustomHeight));
        }
    }
}
