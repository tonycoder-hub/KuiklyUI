/*
 * Tencent is pleased to support the open source community by making KuiklyUI
 * available.
 * Copyright (C) 2026 Tencent. All rights reserved.
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
import com.tencent.kuikly.core.nvi.serialization.json.JSONObject
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNull
import kotlin.test.assertTrue

/**
 * Host unit tests for RenderView.callMethod callback decoding.
 * Covers Boolean/string payloads used by WebView canGoBack / canGoForward.
 */
class RenderViewCallMethodCallbackTest {

    @Test
    fun decodeBooleanJsonTokens() {
        assertEquals(false, decodeViewCallbackPayload("false"))
        assertEquals(true, decodeViewCallbackPayload("true"))
        assertEquals(false, decodeViewCallbackPayload(false))
        assertEquals(true, decodeViewCallbackPayload(true))
    }

    @Test
    fun decodeStringJsonTokens() {
        assertEquals("ok", decodeViewCallbackPayload("\"ok\""))
        assertEquals("canGoBack", decodeViewCallbackPayload("canGoBack"))
        assertEquals("", decodeViewCallbackPayload(""))
    }

    @Test
    fun decodeJsonObjectStillWorks() {
        val parsed = decodeViewCallbackPayload("""{"cursorIndex":3}""")
        val obj = assertIs<JSONObject>(parsed)
        assertEquals(3, obj.optInt("cursorIndex"))
    }

    @Test
    fun decodeNullAndNumberTokens() {
        assertNull(decodeViewCallbackPayload(null))
        assertNull(decodeViewCallbackPayload("null"))
        assertEquals(42, decodeViewCallbackPayload("42"))
        assertEquals(42, decodeViewCallbackPayload(42))
    }

    @Test
    fun canGoBackBooleanPayloadDoesNotThrow() {
        val received = invokeCallMethodCallback("false")
        assertEquals(false, received)
        assertEquals(false, received?.toString() == "true")
    }

    @Test
    fun canGoForwardTrueTokenMatchesWebViewContract() {
        val received = invokeCallMethodCallback("true")
        assertEquals(true, received)
        assertTrue(received?.toString() == "true")
    }

    @Test
    fun rawBooleanCallbackPayload() {
        val received = invokeCallMethodCallback(false)
        assertFalse(received as Boolean)
        assertEquals(true, invokeCallMethodCallback(true))
    }

    @Test
    fun stringCallbackPayload() {
        assertEquals("ready", invokeCallMethodCallback("\"ready\""))
        assertEquals("plain", invokeCallMethodCallback("plain"))
    }

    @Test
    fun jsonObjectCallbackPayload() {
        val received = invokeCallMethodCallback("""{"cursorIndex":7}""")
        val obj = assertIs<JSONObject>(received)
        assertEquals(7, obj.optInt("cursorIndex"))
    }

    private fun invokeCallMethodCallback(nativePayload: Any?): Any? {
        val pagerId = "callback-test-${GlobalFunctions.peekNextRef()}"
        val view = RenderView(pagerId, viewRef = 1, viewName = "KRWebView")
        val callbackRef = GlobalFunctions.peekNextRef().toString()
        var received: Any? = UNSET
        try {
            view.callMethod("canGoBack", null) { result: Any? ->
                received = result
            }
            GlobalFunctions.invokeFunction(pagerId, callbackRef, nativePayload)
            check(received !== UNSET) { "callback was not invoked" }
            return received
        } finally {
            GlobalFunctions.destroyGlobalFunction(pagerId)
        }
    }

    private companion object {
        val UNSET = Any()
    }
}
