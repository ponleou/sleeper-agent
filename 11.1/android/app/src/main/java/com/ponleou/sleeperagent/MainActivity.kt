package com.ponleou.sleeperagent

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import com.ponleou.sleeperagent.ui.theme.SleeperAgentTheme

class MainActivity : ComponentActivity() {
    private val uiState: MutableState<StatusState> = mutableStateOf(StatusState())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            SleeperAgentTheme {
                val logs = NotificationLogRepository.logs.collectAsState().value
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    MainScreen(
                        modifier = Modifier.padding(innerPadding),
                        statusState = uiState.value,
                        logs = logs,
                        onRequestPermission = { requestPostNotifications() },
                        onOpenSettings = { openNotificationAccessSettings() },
                        onStartForeground = { startCollectorForegroundService() }
                    )
                }
            }
        }
        refreshStatus()
    }

    override fun onResume() {
        super.onResume()
        refreshStatus()
    }

    private fun refreshStatus() {
        val hasNotificationAccess = NotificationManagerCompat
            .getEnabledListenerPackages(this)
            .contains(packageName)

        val permissionStatus = if (Build.VERSION.SDK_INT >= 33) {
            if (ContextCompat.checkSelfPermission(
                    this,
                    Manifest.permission.POST_NOTIFICATIONS
                ) == PackageManager.PERMISSION_GRANTED
            ) {
                getString(R.string.permission_granted)
            } else {
                getString(R.string.permission_not_granted)
            }
        } else {
            getString(R.string.permission_not_required)
        }

        val accessStatus = if (hasNotificationAccess) {
            getString(R.string.access_enabled)
        } else {
            getString(R.string.access_disabled)
        }

        uiState.value = StatusState(permissionStatus = permissionStatus, accessStatus = accessStatus)
    }

    private fun requestPostNotifications() {
        if (Build.VERSION.SDK_INT < 33) {
            return
        }

        val permission = Manifest.permission.POST_NOTIFICATIONS
        if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(permission), 100)
        }
    }

    private fun openNotificationAccessSettings() {
        startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
    }

    private fun startCollectorForegroundService() {
        val intent = Intent(this, CollectorForegroundService::class.java)
        ContextCompat.startForegroundService(this, intent)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        refreshStatus()
    }
}

@Composable
fun MainScreen(
    modifier: Modifier = Modifier,
    statusState: StatusState,
    logs: List<LogEntry>,
    onRequestPermission: () -> Unit,
    onOpenSettings: () -> Unit,
    onStartForeground: () -> Unit
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        Text(text = stringResource(R.string.status_title))
        Text(
            text = stringResource(
                R.string.status_format,
                statusState.permissionStatus,
                statusState.accessStatus
            )
        )
        Button(
            onClick = onRequestPermission,
            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 10.dp)
        ) {
            Text(text = stringResource(R.string.action_request_permission))
        }
        Button(
            onClick = onOpenSettings,
            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 10.dp)
        ) {
            Text(text = stringResource(R.string.action_open_settings))
        }
        Button(
            onClick = onStartForeground,
            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 10.dp)
        ) {
            Text(text = stringResource(R.string.action_start_foreground))
        }
        Text(text = stringResource(R.string.status_hint))
        Text(text = stringResource(R.string.logs_title))
        if (logs.isEmpty()) {
            Text(text = stringResource(R.string.logs_empty))
        } else {
            LazyColumn(
                modifier = Modifier.weight(1f, fill = true),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                items(logs) { entry ->
                    Text(text = entry.packageName)
                    if (entry.title.isNotBlank()) {
                        Text(text = entry.title)
                    }
                    if (entry.text.isNotBlank()) {
                        Text(text = entry.text)
                    }
                }
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    SleeperAgentTheme {
        MainScreen(
            statusState = StatusState(
                permissionStatus = "granted",
                accessStatus = "enabled"
            ),
            logs = listOf(
                LogEntry(
                    timestampMillis = 0,
                    packageName = "com.example.app",
                    title = "Example title",
                    text = "Example content"
                )
            ),
            onRequestPermission = {},
            onOpenSettings = {},
            onStartForeground = {}
        )
    }
}

data class StatusState(
    val permissionStatus: String = "",
    val accessStatus: String = ""
)