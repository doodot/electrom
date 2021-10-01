package com.electrom

import android.app.Activity
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.ViewGroup
import com.electrom.extension.appData
import com.electrom.extension.copyElectronAssetFolder
import com.electrom.ipc.IpcBridge
import com.electrom.process.ElectronProcess
import com.electrom.process.MainProcess
import com.electrom.process.RendererProcess
import com.electrom.process.data.BrowserWindowProperty
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.SynchronousQueue
import java.util.concurrent.ThreadPoolExecutor
import java.util.concurrent.TimeUnit


class ElectronApp private constructor(
    internal val activity: Activity,
    internal val viewGroup: ViewGroup
) {

    companion object {
        init {
            System.loadLibrary("electron")
            System.loadLibrary("node")
        }

        fun with(activity: Activity, targetViewGroup: ViewGroup): ElectronApp {
            val context: Context = activity
            // TODO: copy assets files only when changed
            context.copyElectronAssetFolder()
            return ElectronApp(
                activity,
                targetViewGroup
            )
        }

        const val THREAD_POOL_SIZE = 2
    }

    private val threadPoolExecutor = ThreadPoolExecutor(
        THREAD_POOL_SIZE,
        THREAD_POOL_SIZE,
        Long.MAX_VALUE,
        TimeUnit.SECONDS,
        SynchronousQueue()
    )
    private val rendererProcesses: MutableMap<String, ElectronProcess> = ConcurrentHashMap()

    val ipcBridge = IpcBridge()

    internal fun setTitle(title: String) {
        Handler(Looper.getMainLooper()).post {
            activity.title = title
        }
    }

    internal val appData: String
        get() = (activity as Context).appData

    internal fun requestRendererProcess(browserWindowProperty: BrowserWindowProperty): RendererProcess {
        if (rendererProcesses.isNotEmpty()) {
            TODO("throw max renderer process exception")
        }

        val rendererProcess = RendererProcess(this, browserWindowProperty)
        rendererProcesses[rendererProcess.processId] = rendererProcess
        threadPoolExecutor.execute(rendererProcess)
        return rendererProcess
    }

    fun startMainProcess() {
        MainProcess(this, "app.js").execute()
    }

}