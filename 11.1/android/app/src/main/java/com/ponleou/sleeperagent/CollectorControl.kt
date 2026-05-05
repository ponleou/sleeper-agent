package com.ponleou.sleeperagent

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

data class CollectorState(
    val enabled: Boolean,
    val deadlineMillis: Long
)

object CollectorControl {
    private val stateFlow = MutableStateFlow(CollectorState(enabled = false, deadlineMillis = 0L))

    val state: StateFlow<CollectorState> = stateFlow.asStateFlow()

    fun isEnabled(): Boolean {
        return stateFlow.value.enabled
    }

    @Synchronized
    fun setEnabled(value: Boolean) {
        stateFlow.value = if (value) {
            stateFlow.value.copy(enabled = true)
        } else {
            CollectorState(enabled = false, deadlineMillis = 0L)
        }
    }

    @Synchronized
    fun recordActivity(timeoutMillis: Long) {
        val now = System.currentTimeMillis()
        stateFlow.value = CollectorState(enabled = true, deadlineMillis = now + timeoutMillis)
    }
}
