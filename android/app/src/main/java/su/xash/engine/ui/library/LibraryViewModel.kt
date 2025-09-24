package su.xash.engine.ui.library

import android.app.Application
import android.content.Context
import android.content.SharedPreferences
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

    fun reloadGames(ctx: Context) {
        if (isReloading.value == true) {
            return
        }
        _isReloading.value = true

        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                // Önce Android/data dizinini kontrol et
                val internalPath = ctx.getExternalFilesDir(null)?.absolutePath
                val internalDir = File(internalPath ?: "")
                
                // Sonra /xash dizinini kontrol et
                val externalPath = Environment.getExternalStorageDirectory().absolutePath + "/xash"
                val externalDir = File(externalPath)
                
                val games = mutableListOf<Game>()
                
                // Önce internal storage'daki oyunları yükle
                if (internalDir.exists() && internalDir.isDirectory) {
                    games.addAll(Game.getGames(ctx, internalDir))
                }
                
                // Sonra external storage'daki oyunları yükle (çakışmaları önlemek için)
                if (externalDir.exists() && externalDir.isDirectory) {
                    val externalGames = Game.getGames(ctx, externalDir)
                    // Sadece internal'de olmayan oyunları ekle
                    externalGames.forEach { externalGame ->
                        if (!games.any { it.basedir.name == externalGame.basedir.name }) {
                            games.add(externalGame)
                        }
                    }
                }
                
                _installedGames.postValue(games)
                _isReloading.postValue(false)
            }
        }
    }

    fun setSelectedGame(game: Game) {
        _selectedItem.value = game
    }

    fun startEngine(ctx: Context, game: Game) {
        game.startEngine(ctx)
    }
}
