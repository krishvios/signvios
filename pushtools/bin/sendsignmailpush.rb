editrequire 'houston'

# Environment variables are automatically read, or can be overridden by any specified options. You can also
# conveniently use `Houston::Client.development` or `Houston::Client.production`.
APN = Houston::Client.development
APN.certificate = File.read("../../Certificates/iOSPushServicesCertAndKey_EXPIRES_FEB_3_2018.pem")

# An example of the token sent back when a device registers for notifications
#token = "<6f14ca81 bd20a15a 597943cc 05792b76 80993f69 42c0874f 0557b989 371aa302>"
#token = "6f14ca81bd20a15a597943cc05792b7680993f6942c0874f0557b989371aa302"
token = "a59dda7906d466d28c178ae957a5e212d68237d4113f1df8963193abdede256b"

# Create a notification that alerts a message to the user, plays a sound, and sets the badge on the app
notification = Houston::Notification.new(device: token)
notification.alert = "You have a new SignMail message"

# Notifications can also change the badge count, have a custom sound, have a category identifier, indicate available Newsstand content, or pass along arbitrary data.
notification.badge = 57
notification.sound = "sosumi.aiff"
notification.category = "SIGNMAIL_CATEGORY"
notification.content_available = true
notification.custom_data = {DialString: "18015557003", FromBackground: "true"}

# And... sent! That's all it takes.
APN.push(notification)
