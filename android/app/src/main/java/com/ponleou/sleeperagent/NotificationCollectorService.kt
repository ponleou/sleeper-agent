package com.ponleou.sleeperagent

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.os.Build
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat

class NotificationCollectorService : NotificationListenerService() {
    override fun onCreate() {
        super.onCreate()
        if (LOG_NOTIFICATIONS_ENABLED) {
            createLogChannel()
        }
    }

    override fun onListenerConnected() {
        Log.d(TAG, "Notification listener connected")
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        if (!CollectorControl.isEnabled()) {
            return
        }

        if (shouldIgnoreNotification(sbn)) {
            return
        }

        val extras = sbn.notification.extras
        val title = extras.getString(Notification.EXTRA_TITLE) ?: ""
        val text = extras.getCharSequence(Notification.EXTRA_TEXT)?.toString() ?: ""
        val entry = LogEntry(
            timestampMillis = sbn.postTime,
            packageName = sbn.packageName,
            title = title,
            text = text
        )
        val signature = buildString {
            append(sbn.packageName)
            append("|")
            append(title)
            append("|")
            append(text)
        }
        val wasAdded = NotificationLogRepository.upsertIfChanged(entry, sbn.key, signature)
        if (wasAdded) {
            NotificationLogRepository.enqueuePendingPayload(entry)
            if (LOG_NOTIFICATIONS_ENABLED) {
                postLogNotification(entry)
            }
            Log.d(TAG, "pkg=${sbn.packageName} title=$title text=$text")
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        Log.d(TAG, "removed pkg=${sbn.packageName}")
    }

    companion object {
        private const val TAG = "NotifyCollector"
        private const val LOG_CHANNEL_ID = "collector_events"
        private const val LOG_NOTIFICATION_ID = 2000
        private const val LOG_NOTIFICATIONS_ENABLED = false
    }

    private fun createLogChannel() {
        if (android.os.Build.VERSION.SDK_INT >= 26) {
            val channel = NotificationChannel(
                LOG_CHANNEL_ID,
                getString(R.string.log_channel_name),
                NotificationManager.IMPORTANCE_DEFAULT
            )
            val manager = getSystemService(NotificationManager::class.java)
            manager.createNotificationChannel(channel)
        }
    }

    private fun postLogNotification(entry: LogEntry) {
        val stats = NotificationLogRepository.getStatsSnapshot()
        val contentTitle = getString(R.string.log_notification_title)
        val lastApp = if (stats.lastPackage.isBlank()) "none" else stats.lastPackage
        val contentText = "Count: ${stats.uniqueNotifications} | Updates: ${stats.totalUpdates} | Apps: ${stats.uniquePackages} | $lastApp"

        val notification = NotificationCompat.Builder(this, LOG_CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_launcher_foreground)
            .setContentTitle(contentTitle)
            .setContentText(contentText)
            .setStyle(NotificationCompat.BigTextStyle().bigText(contentText))
            .setAutoCancel(true)
            .build()

        NotificationManagerCompat.from(this)
            .notify(LOG_NOTIFICATION_ID, notification)
    }

    private fun shouldIgnoreNotification(sbn: StatusBarNotification): Boolean {
        if (sbn.packageName == packageName) {
            return true
        }

        return Build.VERSION.SDK_INT >= 26 && sbn.notification.channelId == LOG_CHANNEL_ID
    }
}
