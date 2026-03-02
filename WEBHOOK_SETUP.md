# Particle Webhook Setup Guide

This guide explains how to configure a Particle webhook to forward accelerometer data to New Relic.

## Why Webhooks?

Using Particle webhooks provides several benefits:
- **Security**: Credentials stay in the cloud, never on the device or in source code
- **HTTPS Support**: Particle Cloud handles TLS/SSL (the Photon has limited HTTPS support)
- **Easy Updates**: Change credentials without reflashing devices
- **Logging**: Webhook execution logs available in Particle Console

## Prerequisites

- Particle account with at least one device
- New Relic account
- New Relic Insert API key

## Method 1: Web Console (Recommended)

### Step 1: Create Webhook in Particle Console

1. Log in to [console.particle.io](https://console.particle.io)
2. Navigate to **Integrations** in the left sidebar
3. Click **New Integration** → **Webhook**

### Step 2: Basic Configuration

Fill in the following fields:

| Field | Value |
|-------|-------|
| **Event Name** | `nr_accel` |
| **URL** | `https://insights-collector.newrelic.com/v1/accounts/YOUR_ACCOUNT_ID/events` |
| **Request Type** | `POST` |
| **Request Format** | `JSON` |
| **Device** | Select your device or "Any" |

> Replace `YOUR_ACCOUNT_ID` with your actual New Relic account ID

### Step 3: Advanced Settings

Click **Advanced Settings** to expand additional options:

#### HTTP Headers

Add the following headers:

1. Header Name: `X-Insert-Key`
   - Value: `YOUR_INSERT_API_KEY` (your New Relic Insert API key)

2. Header Name: `Content-Type`
   - Value: `application/json`

#### Request Template

Leave as **Default** (pass through device payload as-is).

The device sends data in this format:
```json
[
  {
    "eventType": "AccelSample",
    "timestamp": 1234567890123,
    "accelX": 0.123,
    "accelY": 0.456,
    "accelZ": 0.789,
    "magnitude": 0.935,
    "deviceId": "abc123"
  },
  ...
]
```

### Step 4: Response Template (Optional)

Leave as **Default**. Response data isn't used by the device.

### Step 5: Save

Click **Save** to create the webhook.

## Method 2: Particle CLI

You can also create the webhook using the Particle CLI:

### Step 1: Create Webhook Configuration File

Create a file named `nr-webhook.json`:

```json
{
  "event": "nr_accel",
  "url": "https://insights-collector.newrelic.com/v1/accounts/YOUR_ACCOUNT_ID/events",
  "requestType": "POST",
  "noDefaults": false,
  "rejectUnauthorized": true,
  "headers": {
    "X-Insert-Key": "YOUR_INSERT_API_KEY",
    "Content-Type": "application/json"
  }
}
```

Replace:
- `YOUR_ACCOUNT_ID` with your New Relic account ID
- `YOUR_INSERT_API_KEY` with your New Relic Insert API key

### Step 2: Create Webhook via CLI

```bash
particle webhook create nr-webhook.json
```

### Step 3: Verify

```bash
particle webhook list
```

You should see `nr_accel` in the list.

## Testing the Webhook

### 1. Deploy Device Code

Flash your device with the gee-force firmware:

```bash
particle flash YOUR_DEVICE_NAME
```

### 2. Monitor Serial Output

```bash
particle serial monitor
```

Look for messages like:
```
NR: Publishing 5 samples (598 bytes)
NR: Published successfully (5 samples)
```

### 3. Check Webhook Logs

In Particle Console:
1. Go to **Integrations**
2. Click on your `nr_accel` webhook
3. Click **View Logs**

You should see successful POST requests with 200/201 responses.

### 4. Verify Data in New Relic

Run this NRQL query in New Relic:

```sql
SELECT count(*)
FROM AccelSample
WHERE deviceId = 'YOUR_DEVICE_ID'
SINCE 10 minutes ago
```

You should see sample counts increasing.

## Troubleshooting

### No data appearing in New Relic

1. **Check webhook logs** in Particle Console for HTTP errors
2. **Verify API key** - make sure it's an "Insert" type key
3. **Check account ID** - verify it matches your New Relic account
4. **Rate limits** - Particle.publish() is limited to 1 event/sec burst, 4 events/sec sustained
   - Current config sends 4 batches/second, which is at the limit

### Webhook shows errors

Common errors:

- **403 Forbidden**: Invalid or missing Insert API key
- **404 Not Found**: Incorrect account ID in URL
- **413 Payload Too Large**: Payload exceeds size limits (shouldn't happen with 5 samples)
- **429 Too Many Requests**: Rate limiting (reduce TRANSMIT_INTERVAL)

### Device shows "Publish failed"

- **Cloud not connected**: Device can't reach Particle Cloud
- **Rate limited**: Publishing too frequently (current: 4 Hz is at the limit)
- **Buffer overflow**: Payload exceeds 622 bytes (check serial output)

## Webhook Management

### Update Webhook

To update credentials or configuration:

1. Go to Particle Console → Integrations
2. Click on your webhook
3. Click **Edit**
4. Make changes
5. Click **Save**

Changes take effect immediately (no device reflash needed).

### Delete Webhook

**Web Console:**
1. Integrations → Click webhook → Delete

**CLI:**
```bash
particle webhook delete nr_accel
```

### List All Webhooks

```bash
particle webhook list
```

## Security Best Practices

1. **Never commit credentials** to version control
2. **Use Insert-only keys** (not User keys with broader permissions)
3. **Rotate keys periodically** - update webhook configuration when you do
4. **Monitor webhook logs** for suspicious activity
5. **Use device-specific webhooks** if possible (set Device field instead of "Any")

## Rate Limits

**Particle.publish():**
- Burst: 1 event/second
- Sustained: 4 events/second (average over 1 minute)
- Data size: 622 bytes maximum

**Current Configuration:**
- Batch size: 5 samples
- Transmit rate: 4 Hz (4 batches/second)
- Payload size: ~600 bytes
- **Status: At sustained rate limit** ⚠️

If you experience rate limiting, reduce `TRANSMIT_INTERVAL` or `NR_MAX_SAMPLES`.

## Additional Resources

- [Particle Webhooks Documentation](https://docs.particle.io/reference/cloud-apis/webhooks/)
- [New Relic Event API Documentation](https://docs.newrelic.com/docs/data-apis/ingest-apis/event-api/introduction-event-api/)
- [Particle.publish() Reference](https://docs.particle.io/reference/device-os/api/cloud-functions/particle-publish/)
