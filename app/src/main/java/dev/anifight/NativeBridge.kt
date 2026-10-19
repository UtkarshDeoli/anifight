package dev.anifight

import android.content.res.AssetManager

/**
 * The one and only door into the native engine.
 *
 * Everything here must be called on the GL thread, which is what a
 * [android.opengl.GLSurfaceView] renderer callback already runs on.
 */
internal object NativeBridge {

    init {
        System.loadLibrary("anifight")
    }

    /**
     * @param settingsPath where the engine may save its settings -- the on-screen
     *   pad's layout. The engine has no idea where an app is allowed to write, so it
     *   is told; an empty string means the pad still works but forgets its
     *   arrangement when the app closes.
     * @return true if the engine started and owns a GL context.
     */
    external fun create(assets: AssetManager, width: Int, height: Int, settingsPath: String): Boolean

    external fun resize(width: Int, height: Int)

    external fun frame(deltaSeconds: Float)

    /**
     * One contact point, in device pixels with a top-left origin.
     *
     * `id` is Android's pointer id, which stays the same for as long as the
     * finger is down. The engine keeps a live contact per id so that holding a
     * direction with one thumb and pressing an attack with the other both work.
     */
    external fun pointer(id: Int, x: Float, y: Float, down: Boolean)

    /** Index into [InputButton.ALL]. */
    external fun button(index: Int, down: Boolean)

    external fun destroy()

    /**
     * Drops the engine without touching OpenGL. Use only when the GL context is
     * already destroyed, where calling `destroy()` would delete objects that no
     * longer belong to any context.
     */
    external fun detach()
}

/**
 * The physical buttons on the on-screen pad. The indices are the contract with
 * `buttonForIndex()` in the native bridge; do not reorder.
 */
internal enum class InputButton {
    Up, Down, Left, Right,
    A, B, C,
    X, Y, Z,
    Start;

    companion object {
        val ALL = entries.toTypedArray()
    }
}
