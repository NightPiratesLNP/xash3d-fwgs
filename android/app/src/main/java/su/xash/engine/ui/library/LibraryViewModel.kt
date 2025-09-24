package su.xash.engine.ui.library

import android.app.Application
import android.content.Context
import android.content.SharedPreferences
import android.preference.PreferenceManager
import android.os.Environment
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import su.xash.engine.model.Game
import java.io.File

class LibraryViewModel(application: Application) : AndroidViewModel(application) {
    val installedGames: LiveData<List<Game>> get() = _installedGames
    private val _installedGames = MutableLiveData(emptyList<Game>())

    val isReloading: LiveData<Boolean> get() = _isReloading
    private val _isReloading = MutableLiveData(false)

    val selectedItem: LiveData<Game> get() = _selectedItem
    private val _selectedItem = MutableLiveData<Game>()

    private val appPreferences: SharedPreferences =
        application.getSharedPreferences("app_preferences", Context.MODE_PRIVATE)
    
    private val defaultPreferences: SharedPreferences =
        PreferenceManager.getDefaultSharedPreferences(application)

    // Orijinal reloadGames metodu (geriye uyumluluk için)
    fun reloadGames(ctx: Context) {
        reloadGames(ctx, getStoragePath())
    }

    // Yeni reloadGames metodu (path parametresi ile)
    fun reloadGames(ctx: Context, path: String? = null) {
        if (isReloading.value == true) {
            return
        }
        _isReloading.value = true

        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                val rootPath = path ?: getStoragePath()
                val root = File(rootPath)

                _installedGames.postValue(Game.getGames(ctx, root))
                _isReloading.postValue(false)
            }
        }
    }

    // Storage path'ini almak için yardımcı fonksiyon
    private fun getStoragePath(): String {
        val useInternalStorage = defaultPreferences.getBoolean("storage_toggle", false)
        
        return if (useInternalStorage) {
            // Android/data path'i kullan
            getApplication<Application>().getExternalFilesDir(null)?.absolutePath 
                ?: Environment.getExternalStorageDirectory().absolutePath + "/Android/data/su.xash.engine.test/files"
        } else {
            // External xash klasörü kullan
            Environment.getExternalStorageDirectory().absolutePath + "/xash"
        }
    }

    fun setSelectedGame(game: Game) {
        _selectedItem.value = game
    }

    fun startEngine(ctx: Context, game: Game) {
        game.startEngine(ctx)
    }
}
