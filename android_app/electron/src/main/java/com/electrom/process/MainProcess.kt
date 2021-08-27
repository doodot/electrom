package com.electrom.process

import android.util.Log
import com.electrom.ElectronApp
import com.electrom.extension.LOG_TAG
import com.electrom.extension.appData
import com.electrom.extension.toObject
import com.electrom.ipc.model.IpcMessage
import java.util.*

class MainProcess(
    private val electronApp: ElectronApp,
    private val mainPath: String
) : ElectronProcess {

    override val processId: String = UUID.randomUUID().toString()

    private external fun startMainModule(arguments: Array<String>): Int

    private fun sendTo(sourceId: String, data: String) {
        electronApp.ipcBridge.mainSendTo(
            IpcMessage(
                originId = processId,
                targetId = sourceId,
                data = data
            )
        )
    }

    private fun startRendererProcess(properties: String) {
        Log.d("electron", "Renderer process started by $processId")
        electronApp.requestRendererProcess(properties.toObject())
    }

    override fun run() {
        Log.d(LOG_TAG, "Main process started in $processId")
        startMainModule(
            arrayOf(
                "node", "${electronApp.context.appData}/$mainPath"
            )
        )
    }
}