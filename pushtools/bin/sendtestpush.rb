require 'houston'

# Environment variables are automatically read, or can be overridden by any specified options. You can also
# conveniently use `Houston::Client.development` or `Houston::Client.production`.
use_dev_server = true

if use_dev_server
  APN = Houston::Client.development
  APN.certificate = File.read("../../Certificates/iOSPushServicesCertAndKey_EXPIRES_FEB_3_2018.pem")
else
  APN = Houston::Client.production
  APN.certificate = File.read("../../Certificates/iOSPushServicesCertAndKey_EXPIRES_FEB_3_2018.pem")
end

# An example of the token sent back when a device registers for notifications
#token = "12d0aaa05ee808011ad4bc51cb2c527d359d2c5341b9edf2bcbe60043fc03462" #jbriggs
#token = "338040def8147b848c0162373a1f51776e84e4b0d75c3ffe75c8a9cd4b0ff0a6"
#token = "b6cb5a0b8b21eb05bf00ee3b8e6512bbb884faa8b57b78d2c73a3e003aaa99e8"
token = "40c38424f8af88a9fe4903240442ab77e5a5b3d9ea950c501ddf37a0ff59d207"

# Create a notification that alerts a message to the user, plays a sound, and sets the badge on the app
notification = Houston::Notification.new(device: token)
notification.alert = 'You have a new SignMail from Kevin Selman'

# Notifications can also change the badge count, have a custom sound, have a category identifier, indicate available Newsstand content, or pass along arbitrary data.
notification.badge = 57
notification.sound = 'sosumi.aiff'
notification.category = 'SIGNMAIL_CATEGORY'
notification.content_available = true
notification.mutable_content = true
notification.custom_data = { attachment-url: "https://framework.realtime.co/blog/img/ios10-video.mp4" }

# And... sent! That's all it takes.
APN.push(notification)
