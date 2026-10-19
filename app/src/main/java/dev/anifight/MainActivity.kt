package dev.anifight

import android.app.Activity
import android.os.Build
import android.os.Bundle
import android.view.WindowInsets
import android.view.WindowInsetsController
import android.view.WindowManager

/**
 * The host for [GameView]. Deliberately thin: a fighting game has no Android
 * UI beyond the game surface itself.
 */
class MainActivity : Activity() {

    private lateinit var gameView: GameView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // A match is played in real time; letting the screen dim mid-round would
        // drop the player's inputs.
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        gameView = GameView(this)
        setContentView(gameView)
        // The view handles key events, so it needs the focus on a device with a
        // keyboard or a gamepad attached.
        gameView.requestFocus()
        enterImmersiveMode()
    }

    override fun onResume() {
        super.onResume()
        gameView.onResume()
        enterImmersiveMode()
    }

    override fun onPause() {
        gameView.onPause()
        super.onPause()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        // The bars come back whenever the notification shade is pulled down.
        if (hasFocus) enterImmersiveMode()
    }

    override fun onDestroy() {
        // By now the GL thread may already be stopped. Hand the engine back on
        // the GL thread if it still exists, otherwise drop it.
        if (isFinishing) {
            gameView.detachWithoutContext()
        } else {
            gameView.releaseOnGlThread()
        }
        super.onDestroy()
    }

    /** Hides the status and navigation bars so the game gets the whole screen. */
    private fun enterImmersiveMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false)
            window.insetsController?.apply {
                hide(WindowInsets.Type.systemBars())
                systemBarsBehavior =
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            }
        } else {
            @Suppress("DEPRECATION")
            window.decorView.systemUiVisibility = (
                android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    or android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    or android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    or android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    or android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
                    or android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                )
        }
    }

    /** Back closes the game; MUGEN has no Android-style back stack. */
    @Deprecated("Handled for API < 33")
    override fun onBackPressed() {
        finish()
    }
}
