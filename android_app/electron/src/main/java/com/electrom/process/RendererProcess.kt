package com.electrom.process

import android.graphics.Color
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.ViewGroup
import com.electrom.ElectronApp
import com.electrom.process.data.BrowserWindowProperty
import com.electrom.view.ElectronWebView
import java.util.*
import java.util.concurrent.CountDownLatch

class RendererProcess(
    private val electronApp: ElectronApp,
    private val browserWindowProperties: BrowserWindowProperty
) : ElectronProcess {

    override val processId: String = UUID.randomUUID().toString()
    private lateinit var webView: ElectronWebView

    private inline fun awaitMainLooper(crossinline block: () -> Unit) {
        val wg = CountDownLatch(1)
        Handler(Looper.getMainLooper()).post {
            block()
            wg.countDown()
        }
        wg.await()
    }

    private fun attachWebViewOnStart() {
        awaitMainLooper {
            webView = ElectronWebView(electronApp).apply {
                layoutParams = ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT
                )
            }

            browserWindowProperties.run {
                backgroundColor?.also {
                    webView.setBackgroundColor(Color.parseColor(it))
                }

                show?.also {
                    if (!show) {
                        webView.visibility = View.INVISIBLE
                    }
                }

                electronApp.viewGroup.addView(webView)
            }
        }
    }

    internal fun loadUrl(url: String) {
        awaitMainLooper {
            webView.loadUrl(url)
        }
    }

    internal fun show() {
        awaitMainLooper {
            webView.visibility = View.VISIBLE
        }
    }

    override fun run() {
        attachWebViewOnStart()
    }
}