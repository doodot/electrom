package com.electrom.process

import android.graphics.Color
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.view.ViewGroup
import com.electrom.ElectronApp
import com.electrom.extension.appData
import com.electrom.process.data.BrowserWindowProperty
import com.electrom.view.ElectronWebView
import java.io.File
import java.util.*
import java.util.concurrent.CountDownLatch

class RendererProcess(
    private val electronApp: ElectronApp,
    private val browserWindowProperties: BrowserWindowProperty
) : ElectronProcess {

    override val processId: String = UUID.randomUUID().toString()

    private fun attachWebViewOnStart() {
        val wg = CountDownLatch(1)
        Handler(Looper.getMainLooper()).post {
            val electronWebView = ElectronWebView(electronApp).apply {
                layoutParams = ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT
                )
            }
            electronApp.viewGroup.addView(electronWebView)

            browserWindowProperties.run {
                backgroundColor?.also {
                    electronWebView.setBackgroundColor(Color.parseColor(it))
                }
            }

            val externalTarget = "${electronApp.context.appData}/electron_app/index.html"

            electronWebView.loadUrl(Uri.fromFile(File(externalTarget)).toString())
            wg.countDown()
        }
        wg.await()
    }

    override fun run() {
        attachWebViewOnStart()
    }
}