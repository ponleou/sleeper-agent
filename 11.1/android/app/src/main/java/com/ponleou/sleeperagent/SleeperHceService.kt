package com.ponleou.sleeperagent

import android.nfc.cardemulation.HostApduService
import android.os.Bundle
import java.security.SecureRandom
import kotlin.math.min

class SleeperHceService : HostApduService() {
    private var activePayload: PendingPayload? = null
    private var activeOffset: Int = 0

    override fun processCommandApdu(commandApdu: ByteArray, extras: Bundle?): ByteArray {
        if (isSelectAid(commandApdu)) {
            return NfcApdu.SW_SUCCESS
        }

        if (isIdentifyCommand(commandApdu)) {
            val deviceId = getStoredDeviceId().toByteArray(Charsets.US_ASCII)
            return deviceId + NfcApdu.SW_SUCCESS
        }

        if (isPollCommand(commandApdu)) {
            return buildPollResponse()
        }

        if (isCollectCommand(commandApdu)) {
            return buildCollectResponse()
        }

        return NfcApdu.SW_UNKNOWN
    }

    override fun onDeactivated(reason: Int) {
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
        response[0] = chunkSize.toByte()
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
        private const val PREFS_NAME = "sleeper_agent_prefs"
        private const val PREF_DEVICE_ID = "device_id"
        private const val DEVICE_ID_LENGTH = 6

        private val SECURE_RANDOM = SecureRandom()
        private val NANOID_ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_".toCharArray()
    }
}
