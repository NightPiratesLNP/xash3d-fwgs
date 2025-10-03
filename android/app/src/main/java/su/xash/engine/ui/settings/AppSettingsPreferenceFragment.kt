package su.xash.engine.ui.settings

import android.app.AlertDialog
import android.os.Bundle
import android.widget.EditText
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import su.xash.engine.MainActivity
import su.xash.engine.R
import android.content.SharedPreferences
import android.preference.PreferenceManager

class AppSettingsPreferenceFragment() : PreferenceFragmentCompat(),
    SharedPreferences.OnSharedPreferenceChangeListener {

    private lateinit var preferences: SharedPreferences
    private lateinit var gamePathPreference: Preference
    private lateinit var globalArgsPreference: Preference
    private lateinit var resolutionWidthPreference: Preference
    private lateinit var resolutionHeightPreference: Preference
    private lateinit var resolutionScalePreference: Preference

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        preferenceManager.sharedPreferencesName = "app_preferences"
        setPreferencesFromResource(R.xml.app_preferences, rootKey)

        preferences = PreferenceManager.getDefaultSharedPreferences(requireContext())
        preferences.registerOnSharedPreferenceChangeListener(this)

        gamePathPreference = findPreference("game_path") ?: return
        globalArgsPreference = findPreference("global_arguments") ?: return
        resolutionWidthPreference = findPreference("resolution_width") ?: return
        resolutionHeightPreference = findPreference("resolution_height") ?: return
        resolutionScalePreference = findPreference("resolution_scale") ?: return

        globalArgsPreference.setOnPreferenceClickListener {
            showGlobalArgumentsDialog()
            true
        }

        resolutionWidthPreference.setOnPreferenceClickListener {
            showResolutionDialog("width", getString(R.string.resolution_width_dialog))
            true
        }

        resolutionHeightPreference.setOnPreferenceClickListener {
            showResolutionDialog("height", getString(R.string.resolution_height_dialog))
            true
        }

        resolutionScalePreference.setOnPreferenceClickListener {
            showResolutionDialog("scale", getString(R.string.resolution_scale_dialog))
            true
        }

        updateGamePathSummary()
        updateGlobalArgsSummary()
        updateResolutionSummaries()
    }

    override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences, key: String?) {
        when (key) {
            "storage_toggle" -> {
                updateGamePathSummary()
            }
            "global_arguments" -> {
                updateGlobalArgsSummary()
            }
            "resolution_width", "resolution_height", "resolution_scale" -> {
                updateResolutionSummaries()
            }
        }
    }

    private fun updateGamePathSummary() {
        (activity as? MainActivity)?.let { mainActivity ->
            gamePathPreference.summary = mainActivity.getStorageSummary()
        } ?: run {
            val useInternalStorage = preferences.getBoolean("storage_toggle", false)
            gamePathPreference.summary = if (useInternalStorage) {
                "Internal Storage (Android/data)"
            } else {
                "External Storage (/storage/emulated/0/xash)"
            }
        }
    }

    private fun updateGlobalArgsSummary() {
        val globalArgs = preferences.getString("global_arguments", "")
        if (globalArgs.isNullOrEmpty()) {
            globalArgsPreference.summary = "No global arguments set"
        } else {
            globalArgsPreference.summary = globalArgs
        }
    }

    private fun updateResolutionSummaries() {
        val width = preferences.getString("resolution_width", "0") ?: "0"
        val height = preferences.getString("resolution_height", "0") ?: "0"
        val scale = preferences.getString("resolution_scale", "1.0") ?: "1.0"

        if (width != "0" && height != "0") {
            resolutionWidthPreference.summary = "Width: $width"
            resolutionHeightPreference.summary = "Height: $height"
            resolutionScalePreference.summary = getString(R.string.resolution_disabled)
        } else if (scale != "1.0") {
            val metrics = resources.displayMetrics
            val scaledWidth = (metrics.widthPixels / scale.toFloat()).toInt()
            val scaledHeight = (metrics.heightPixels / scale.toFloat()).toInt()
            
            resolutionScalePreference.summary = getString(R.string.resolution_scaled, scale.toFloat(), scaledWidth, scaledHeight)
            resolutionWidthPreference.summary = "Native: ${metrics.widthPixels}"
            resolutionHeightPreference.summary = "Native: ${metrics.heightPixels}"
        } else {
            resolutionWidthPreference.summary = getString(R.string.resolution_disabled)
            resolutionHeightPreference.summary = getString(R.string.resolution_disabled)
            resolutionScalePreference.summary = getString(R.string.resolution_disabled)
        }
    }

    private fun showGlobalArgumentsDialog() {
        val currentArgs = preferences.getString("global_arguments", "") ?: ""
        
        val editText = EditText(requireContext())
        editText.setText(currentArgs)
        editText.hint = "e.g., -dev -log"
        
        AlertDialog.Builder(requireContext())
            .setTitle("Global Command-line Arguments")
            .setMessage("These arguments will be added to all games")
            .setView(editText)
            .setPositiveButton("OK") { dialog, which ->
                val newArgs = editText.text.toString().trim()
                preferences.edit().putString("global_arguments", newArgs).commit()
                updateGlobalArgsSummary()
            }
            .setNegativeButton("Cancel", null)
            .setNeutralButton("Clear") { dialog, which ->
                preferences.edit().putString("global_arguments", "").commit()
                updateGlobalArgsSummary()
            }
            .show()
    }

    private fun showResolutionDialog(type: String, dialogTitle: String) {
        val currentValue = when (type) {
            "width" -> preferences.getString("resolution_width", "0") ?: "0"
            "height" -> preferences.getString("resolution_height", "0") ?: "0"
            "scale" -> preferences.getString("resolution_scale", "1.0") ?: "1.0"
            else -> "0"
        }
        
        val editText = EditText(requireContext())
        editText.setText(currentValue)
        
        AlertDialog.Builder(requireContext())
            .setTitle(dialogTitle)
            .setView(editText)
            .setPositiveButton("OK") { dialog, which ->
                val newValue = editText.text.toString().trim()
                when (type) {
                    "width" -> {
                        preferences.edit().putString("resolution_width", newValue).commit()
                        if (newValue != "0" && preferences.getString("resolution_height", "0") == "0") {
                            val metrics = resources.displayMetrics
                            val aspectRatio = metrics.heightPixels.toFloat() / metrics.widthPixels.toFloat()
                            val autoHeight = (newValue.toInt() * aspectRatio).toInt()
                            preferences.edit().putString("resolution_height", autoHeight.toString()).commit()
                        }
                    }
                    "height" -> preferences.edit().putString("resolution_height", newValue).commit()
                    "scale" -> preferences.edit().putString("resolution_scale", newValue).commit()
                }
                updateResolutionSummaries()
            }
            .setNegativeButton("Cancel", null)
            .setNeutralButton("Clear") { dialog, which ->
                when (type) {
                    "width" -> preferences.edit().putString("resolution_width", "0").commit()
                    "height" -> preferences.edit().putString("resolution_height", "0").commit()
                    "scale" -> preferences.edit().putString("resolution_scale", "1.0").commit()
                }
                updateResolutionSummaries()
            }
            .show()
    }

    override fun onResume() {
        super.onResume()
        preferences.registerOnSharedPreferenceChangeListener(this)
        updateGamePathSummary()
        updateGlobalArgsSummary()
        updateResolutionSummaries()
    }

    override fun onPause() {
        super.onPause()
        preferences.unregisterOnSharedPreferenceChangeListener(this)
    }
}
