package com.electrom.view

import android.annotation.SuppressLint
import android.app.AlertDialog
import android.graphics.Bitmap
import android.webkit.JsResult
import android.webkit.WebChromeClient
import android.webkit.WebView
import android.webkit.WebViewClient
import com.electrom.Electron

@SuppressLint("SetJavaScriptEnabled", "JavascriptInterface", "ViewConstructor")
class ElectronWebView(electron: Electron) : WebView(electron.activity) {

    private val electronInterface: ElectronInterface

    companion object {
        private const val PRE_LOAD_SCRIPT =
            """
            (function(window) {
                const ipcRenderer = new Function();
                
                ipcRenderer.sendTracker = new Map();
                ipcRenderer.handler = new Map();
                
                ipcRenderer.sendSync = function(channel, data) {
                    return window['@@android'].ipcRendererSendSync(channel, data);
                }
                
                ipcRenderer.on = function(channel, cb) {
                    if (!this.handler.has(channel)) {
                        this.handler.add(channel, cb);
                    }
                }
                
                ipcRenderer.emit = function(channel, event) {
                    const cb = this.handler.get(channel);
                    if (cb !== undefined) {
                        cb(event);
                    }
                }
                
                ipcRenderer.send = function(channel, data) {
                    window['@@android'].ipcRendererSend(channel, data);
                }
                
                window['@@electron'] = {
                    ipcRenderer
                }

                window.require = function(path) {
                    if (path === 'electron') {
                        return window['@@electron'];
                    } else {
                        throw new Error('Module not found ' + path)
                    }
                };
                
                window.module = {}
            })(window);
            """
    }

    init {
        settings.apply {
            javaScriptEnabled = true
            allowFileAccess = true

            // Resolve CORS policy for http requests
            // https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/cors-and-webview-api.md
            allowUniversalAccessFromFileURLs = true

            webViewClient = object : WebViewClient() {
                override fun onPageFinished(view: WebView, url: String) {
                    electron.setTitle(view.title ?: "<electron>")
                }

                override fun onPageStarted(view: WebView?, url: String?, favicon: Bitmap?) {
                    evaluateJavascript(PRE_LOAD_SCRIPT, null)
                }
            }
            webChromeClient = object : WebChromeClient() {
                override fun onJsAlert(
                    view: WebView?,
                    url: String?,
                    message: String?,
                    result: JsResult
                ): Boolean {
                    AlertDialog.Builder(context).apply {
                        setTitle("Alert")
                        setMessage(message)
                        setPositiveButton("Yes") { _, _ ->
                            result.confirm()
                        }
                    }.setCancelable(false).create().show()
                    return true
                }
            }

            setWebContentsDebuggingEnabled(true)
        }
        electronInterface = ElectronInterface(context)
        addJavascriptInterface(electronInterface, "@@android")
    }
}