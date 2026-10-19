package dev.anifight

import android.content.Context
import android.opengl.GLSurfaceView
import android.view.KeyEvent
import android.view.MotionEvent
import java.io.File
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

/**
 * The single view the game is drawn into.
 *
 * This class is the renderer as well as the view, because the only thing it
 * does is forward the GL thread's callbacks into the native engine. All game
 * logic lives in C++.
 */
internal class GameView(context: Context) : GLSurfaceView(context), GLSurfaceView.Renderer {

    private var engineStarted = false
    private var lastFrameNanos = 0L

    init {
        setEGLContextClientVersion(3)
        // MUGEN is 2D: no depth buffer, no stencil, no alpha in the surface.
        // 8/8/8/0 keeps the framebuffer small so the per-frame swap stays cheap
        // on low-end hardware.
        setEGLConfigChooser(8, 8, 8, 0, 0, 0)
        // Surviving a pause without rebuilding the screenpack is worth it: the
        // engine caches every sprite as a GPU texture.
        preserveEGLContextOnPause = true
        setRenderer(this)
        renderMode = RENDERMODE_CONTINUOUSLY

        // A physical keyboard or a gamepad maps onto the same virtual pad the
        // on-screen controls will drive, so the game is playable and scriptable
        // without one.
        isFocusable = true
        isFocusableInTouchMode = true
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        val button = buttonForKeyCode(keyCode) ?: return super.onKeyDown(keyCode, event)
        NativeBridge.button(button.ordinal, true)
        return true
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        val button = buttonForKeyCode(keyCode) ?: return super.onKeyUp(keyCode, event)
        NativeBridge.button(button.ordinal, false)
        return true
    }

    private fun buttonForKeyCode(keyCode: Int): InputButton? = when (keyCode) {
        KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_W -> InputButton.Up
        KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_S -> InputButton.Down
        KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_A -> InputButton.Left
        KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_D -> InputButton.Right
        KeyEvent.KEYCODE_J -> InputButton.A
        KeyEvent.KEYCODE_K -> InputButton.B
        KeyEvent.KEYCODE_L -> InputButton.C
        KeyEvent.KEYCODE_U -> InputButton.X
        KeyEvent.KEYCODE_I -> InputButton.Y
        KeyEvent.KEYCODE_O -> InputButton.Z
        KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_SPACE, KeyEvent.KEYCODE_BUTTON_START ->
            InputButton.Start
        else -> null
    }

    // ---------------------------------------------------------------- renderer

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        lastFrameNanos = 0L

        if (engineStarted) {
            // The EGL context was lost while we were paused. Every texture the
            // engine holds is now invalid, so start over rather than drawing
            // with dangling handles.
            NativeBridge.destroy()
            engineStarted = false
        }

        // The real size arrives in onSurfaceChanged.
        engineStarted = NativeBridge.create(
                context.assets,
                1,
                1,
                // The engine's own settings live beside the app's, in the same place
                // Android puts everything else this app is allowed to write.
                File(context.filesDir, "anifight.ini").absolutePath,
            )
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        if (engineStarted) {
            NativeBridge.resize(width, height)
        }
    }

    override fun onDrawFrame(gl: GL10?) {
        if (!engineStarted) return

        val now = System.nanoTime()
        val deltaSeconds =
            if (lastFrameNanos == 0L) 0f else (now - lastFrameNanos) / 1_000_000_000f
        lastFrameNanos = now

        NativeBridge.frame(deltaSeconds)
    }

    // ------------------------------------------------------------------- input

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (!engineStarted) return false

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                val i = event.actionIndex
                NativeBridge.pointer(event.getPointerId(i), event.getX(i), event.getY(i), true)
            }

            MotionEvent.ACTION_MOVE -> {
                // Every finger is reported on every move, so a contact that slides
                // from one button to the next stays tracked rather than sticking.
                for (i in 0 until event.pointerCount) {
                    NativeBridge.pointer(
                        event.getPointerId(i), event.getX(i), event.getY(i), true,
                    )
                }
            }

            MotionEvent.ACTION_POINTER_UP -> {
                val i = event.actionIndex
                NativeBridge.pointer(event.getPointerId(i), event.getX(i), event.getY(i), false)
            }

            MotionEvent.ACTION_UP -> {
                NativeBridge.pointer(event.getPointerId(0), event.getX(0), event.getY(0), false)
                performClick()
            }

            MotionEvent.ACTION_CANCEL -> {
                // The gesture was taken from us; we get no per-pointer ups, so
                // release everything the event still knows about.
                for (i in 0 until event.pointerCount) {
                    NativeBridge.pointer(
                        event.getPointerId(i), event.getX(i), event.getY(i), false,
                    )
                }
            }
        }
        return true
    }

    /** Satisfies the accessibility contract; the game has nothing to click. */
    override fun performClick(): Boolean = super.performClick()

    /**
     * Releases the engine while the GL thread (and therefore the GL context) is
     * still alive, so its texture deletes are legal.
     */
    fun releaseOnGlThread() {
        if (!engineStarted) return
        queueEvent {
            NativeBridge.destroy()
        }
        engineStarted = false
    }

    /**
     * Releases the engine when the context is already gone. Nothing can be
     * deleted against a dead context, so the native side just drops its handles.
     */
    fun detachWithoutContext() {
        if (!engineStarted) return
        NativeBridge.detach()
        engineStarted = false
    }
}
