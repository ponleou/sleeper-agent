package com.ponleou.sleeperagent

import android.nfc.cardemulation.HostApduService
import android.os.Bundle
import java.security.SecureRandom

class SleeperHceService : HostApduService() {
    override fun processCommandApdu(commandApdu: ByteArray, extras: Bundle?): ByteArray {
        if (isSelectAid(commandApdu)) {
            return NfcApdu.SW_SUCCESS
        }

        if (isIdentifyCommand(commandApdu)) {
            val deviceId = getStoredDeviceId().toByteArray(Charsets.US_ASCII)
            return deviceId + NfcApdu.SW_SUCCESS
        }

        if (isPollCommand(commandApdu)) {
            return NfcApdu.SW_SUCCESS
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
