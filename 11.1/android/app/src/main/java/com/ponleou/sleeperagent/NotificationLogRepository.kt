package com.ponleou.sleeperagent

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

object NotificationLogRepository {
    private const val MAX_ENTRIES = 200
    private val logState = MutableStateFlow<List<LogEntry>>(emptyList())
    private val lastContentByKey = LinkedHashMap<String, String>()
    private val entriesByKey = LinkedHashMap<String, LogEntry>()
    private val packageByKey = LinkedHashMap<String, String>()
    private val statsState = MutableStateFlow(LogStats())

    val logs: StateFlow<List<LogEntry>> = logState.asStateFlow()
    val stats: StateFlow<LogStats> = statsState.asStateFlow()

    fun getStatsSnapshot(): LogStats {
        return statsState.value
    }

    @Synchronized
    fun upsertIfChanged(entry: LogEntry, key: String, contentSignature: String): Boolean {
        val lastSignature = lastContentByKey[key]
        if (lastSignature == contentSignature) {
            return false
        }
        lastContentByKey[key] = contentSignature

        val existingEntry = entriesByKey[key]
        val mergedEntry = if (existingEntry == null) {
            entry
        } else {
            val mergedTitle = if (entry.title.isNotBlank()) entry.title else existingEntry.title
            val mergedText = when {
                entry.text.isBlank() -> existingEntry.text
                existingEntry.text.isBlank() -> entry.text
                else -> existingEntry.text + "\n" + entry.text
            }
            entry.copy(title = mergedTitle, text = mergedText)
        }

        val updatedEntries = LinkedHashMap<String, LogEntry>()
        updatedEntries[key] = mergedEntry
        for ((existingKey, existingEntry) in entriesByKey) {
            if (existingKey != key) {
                updatedEntries[existingKey] = existingEntry
            }
        }

        entriesByKey.clear()
        entriesByKey.putAll(updatedEntries)
        if (!packageByKey.containsKey(key)) {
            packageByKey[key] = entry.packageName
        }
        trimCache()
        logState.value = entriesByKey.values.toList()
        updateStats(entry)
        return true
    }

    private fun trimCache() {
        if (entriesByKey.size <= MAX_ENTRIES) {
            return
        }

        val iterator = entriesByKey.entries.iterator()
        var index = 0
        while (iterator.hasNext()) {
            val entry = iterator.next()
            index += 1
            if (index > MAX_ENTRIES) {
                iterator.remove()
                lastContentByKey.remove(entry.key)
                packageByKey.remove(entry.key)
            }
        }
    }

    private fun updateStats(entry: LogEntry) {
        val totalUpdates = statsState.value.totalUpdates + 1
        val uniqueNotifications = entriesByKey.size
        val uniquePackages = packageByKey.values.toHashSet().size
        statsState.value = LogStats(
            totalUpdates = totalUpdates,
            uniqueNotifications = uniqueNotifications,
            uniquePackages = uniquePackages,
            lastPackage = entry.packageName,
            lastTitle = entry.title,
            lastUpdatedMillis = entry.timestampMillis
        )
    }
}

data class LogEntry(
    val timestampMillis: Long,
    val packageName: String,
    val title: String,
    val text: String
)

data class LogStats(
    val totalUpdates: Int = 0,
    val uniqueNotifications: Int = 0,
    val uniquePackages: Int = 0,
    val lastPackage: String = "",
    val lastTitle: String = "",
    val lastUpdatedMillis: Long = 0
)
