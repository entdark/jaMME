using System;
using System.Threading.Tasks;

using Android.App;
using Android.Content;
using Android.OS;
using Java.Interop;

namespace android {
	[Service(Name = "com.jamme.jaMMEService", Enabled = true)]
	[IntentFilter(new string[] { "com.jamme.jaMMEService" })]
	public class jaMMEService : Service
	{
		private int notificationId = 3;
		private Handler handler = new Handler();
//		private static Context Context = null;
//		[Export]
		public/* static*/ void showNotification(string message) {
			if (!jaMME.ServiceRunning)
				return;
			if (string.IsNullOrEmpty(message))
				return;
			handler.Post(() => {
				var openjaMMEIntent = new Intent(this, typeof(jaMME));
				var pendingIntent = PendingIntent.GetActivity(this, 1337, openjaMMEIntent, PendingIntentFlags.OneShot);
				var notification = new Notification.Builder(this)
					.SetContentTitle(message)
					.SetContentText("Tap to resume")
					.SetSmallIcon(Resource.Drawable.icon)
					.SetContentIntent(pendingIntent)
					.SetAutoCancel(true)
					.Build();

				var notificationManager = NotificationManager.FromContext(this);
				notificationManager.Notify(notificationId++, notification);
			});
		}
		public override IBinder OnBind(Intent intent) {
			return new Binder();
		}
		public override StartCommandResult OnStartCommand(Intent intent, StartCommandFlags flags, int startId) {
//			jaMMEService.Context = this;
			//update the game with 20fps (1000 / 50)
			jaMME.ServiceRunning = true;
			var openjaMMEIntent = new Intent(this, typeof(jaMME));
			var pendingIntent = PendingIntent.GetActivity(this, 1337, openjaMMEIntent, PendingIntentFlags.OneShot);
			var notification = new Notification.Builder(this)
				.SetContentTitle("jaMME is running on background")
				.SetContentText("Tap to resume")
				.SetSmallIcon(Resource.Drawable.icon)
				.SetContentIntent(pendingIntent)
				.Build();
			StartForeground(2, notification);
			Task.Run(async () => {
				while (true) {
					await Task.Delay(50);
					if (jaMME.KillService || !jaMME.GameRunning) {
						jaMME.KillService = false;
						jaMME.ServiceRunning = false;
						break;
					}
					int jammeFlags = jaMME.frame();
					if (jaMME.hasNotifications() != 0)
						showNotification(jaMME.getNotificationMsg());
				}
				this.StopSelf();
			});
			return StartCommandResult.Sticky;
		}
		public override void OnDestroy() {
			jaMME.KillService = false;
			jaMME.ServiceRunning = false;
//			jaMMEService.Context = null;
			base.OnDestroy();
		}
	}
}