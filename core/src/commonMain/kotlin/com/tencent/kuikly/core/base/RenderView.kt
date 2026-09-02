/*
 * Tencent is pleased to support the open source community by making KuiklyUI
 * available.
 * Copyright (C) 2025 Tencent. All rights reserved.
 * Licensed under the License of KuiklyUI;
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://github.com/Tencent-TDS/KuiklyUI/blob/main/LICENSE
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.tencent.kuikly.core.base

import com.tencent.kuikly.core.global.GlobalFunctions
import com.tencent.kuikly.core.layout.Frame
import com.tencent.kuikly.core.manager.BridgeManager
import com.tencent.kuikly.core.manager.PagerManager
import com.tencent.kuikly.core.module.AnyCallbackFn
import com.tencent.kuikly.core.module.CallbackFn
import com.tencent.kuikly.core.module.CallbackRef
import com.tencent.kuikly.core.nvi.serialization.json.JSONEngine
import com.tencent.kuikly.core.nvi.serialization.json.JSONException

class RenderView(private val pagerId: String, private val viewRef: Int, private val viewName: String) {

    var currentFrame: Frame = Frame.zero

    var didLayout = false
       private set
    init {
        BridgeManager.createRenderView(pagerId, viewRef, viewName)
    }

    fun setProp(key: String, value: Any) {
        BridgeManager.setViewProp(pagerId, viewRef, key, value, 0)
    }

    fun setFrame(x: Float, y: Float, width: Float, height: Float) {
        currentFrame = Frame(x, y, width, height)
        BridgeManager.setRenderViewFrame(pagerId, viewRef, x, y, width, height)
        didLayout = true
    }

    fun setEvent(eventName: String, sync: Int = 0) {
        BridgeManager.setViewProp(pagerId, viewRef, eventName, 1, 1, sync)
    }

    fun setShadow() {
        BridgeManager.setShadowForView(pagerId, viewRef)
    }

    fun callMethod(methodName: String, params: String? = null, callback: CallbackFn? = null) {
        var callbackRef: CallbackRef? = null
        val peekedCallbackRef = GlobalFunctions.peekNextRef()
        callback?.also { cb ->
            callbackRef = GlobalFunctions.createFunction(pagerId) { data ->
                val trace = PagerManager.getPagerEventTrace(pagerId)
                trace?.onViewCallbackStart(viewName, viewRef, methodName, peekedCallbackRef)
                val res = decodeViewCallbackPayload(data)
                // Native/WebView methods such as canGoBack return Boolean or String
                // tokens, not JSON objects. CallbackFn stays JSONObject? for
                // source compatibility; invoke through Any so primitives pass.
                @Suppress("UNCHECKED_CAST")
                (cb as AnyCallbackFn)(res)
                trace?.onViewCallbackEnd(viewName, viewRef, methodName, peekedCallbackRef)
                false
            }
        }
        val trace = PagerManager.getPagerEventTrace(pagerId)
        trace?.onViewCallMethodStart(viewName, viewRef, methodName, peekedCallbackRef)
        BridgeManager.callViewMethod(pagerId, viewRef, methodName, params, callbackRef)
        trace?.onViewCallMethodEnd(viewName, viewRef, methodName, peekedCallbackRef)
    }

    fun insertSubRenderView(subViewRef: Int, index: Int) {
        BridgeManager.insertSubRenderView(pagerId, viewRef, subViewRef, index)
    }

    fun insertToRootView() {
        BridgeManager.insertSubRenderView(pagerId, ROOT_VIEW_TAG, viewRef, 0)
    }

    fun removeFromParentRenderView() {
        BridgeManager.removeRenderView(pagerId, viewRef)
    }

    companion object {
        const val ROOT_VIEW_TAG = -1
    }
}

/**
 * Decode a native/WebView callMethod callback payload.
 *
 * Native sides may pass an already-typed value (Boolean, String, Number,
 * JSONObject) or a JSON text token such as `true`, `false`, `"ok"`, or
 * `{...}`. Forcing [JSONObject] throws
 * `Value false of type Boolean cannot be converted to JSONObject`.
 */
internal fun decodeViewCallbackPayload(data: Any?): Any? {
    return when (data) {
        null -> null
        is String -> decodeViewCallbackJsonString(data)
        else -> data
    }
}

internal fun decodeViewCallbackJsonString(json: String): Any? {
    if (json.isEmpty()) {
        return json
    }
    return try {
        JSONEngine.parse(json)
    } catch (_: JSONException) {
        json
    }
}
