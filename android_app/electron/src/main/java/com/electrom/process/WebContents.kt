package com.electrom.process

import android.graphics.Color
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.View
import android.view.ViewGroup
import com.electrom.Electron
import com.electrom.extension.LOG_TAG
import com.electrom.process.data.BrowserWindowProperty
import com.electrom.view.ElectronWebView
import java.util.concurrent.atomic.AtomicInteger

internal class WebContents(
    private val electron: Electron,
    private val browserWindowProperties: BrowserWindowProperty,
    val weakMapId: Int
) {

    private val handler = Handler(Looper.getMainLooper())

    private lateinit var webView: ElectronWebView

    init {
        attachWebViewOnStart()
    }

    private fun attachWebViewOnStart() {
        webView = ElectronWebView(electron).apply {
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

            electron.rendererLayout.addView(webView)
            Log.d(LOG_TAG, "after attach")
        }
    }

    internal fun loadUrl(url: String) {
        webView.loadUrl(url)
    }

    internal fun show() {
        webView.visibility = View.VISIBLE
    }
}