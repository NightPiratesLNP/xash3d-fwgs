package su.xash.engine.ui.settings

import android.os.Bundle
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

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        preferenceManager.sharedPreferencesName = "app_preferences"
        setPreferencesFromResource(R.xml.app_preferences, rootKey)

        preferences = PreferenceManager.getDefaultSharedPreferences(requireContext())
        preferences.registerOnSharedPreferenceChangeListener(this)

        gamePathPreference = findPreference("game_path") ?: return
        
        val globalArgsPreference = findPreference<Preference>("global_arguments")
        globalArgsPreference?.setOnPreferenceClickListener {
            showGlobalArgumentsDialog()
            true
        }

        updateGamePathSummary()
        updateGlobalArgsSummary(globalArgsPreference)
    }

    override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences, key: String?) {
        when (key) {
            "storage_toggle" -> {
                updateGamePathSummary()
            }
            "global_arguments" -> {
                val globalArgsPreference = findPreference<Preference>("global_arguments")
                updateGlobalArgsSummary(globalArgsPreference)
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

    private fun updateGlobalArgsSummary(globalArgsPreference: Preference?) {
        globalArgsPreference?.let { pref ->
            val globalArgs = preferences.getString("global_arguments", "")
            if (globalArgs.isNullOrEmpty()) {
                pref.summary = "No global arguments set"
            } else {
                pref.summary = globalArgs
            }
        }
    }

    private fun showGlobalArgumentsDialog() {
        val globalArgs = preferences.getString("global_arguments", "") ?: ""
        android.util.Log.d("AppSettings", "Global arguments dialog should be shown. Current args: $globalArgs")
    }

    override fun onResume() {
        super.onResume()
        preferences.registerOnSharedPreferenceChangeListener(this)
        updateGamePathSummary()
        val globalArgsPreference = findPreference<Preference>("global_arguments")
        updateGlobalArgsSummary(globalArgsPreference)
    }

    override fun onPause() {
        super.onPause()
        preferences.unregisterOnSharedPreferenceChangeListener(this)
    }
}
