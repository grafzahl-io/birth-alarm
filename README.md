# Birth alarm for horses

A simple and open source solution to get an alarm when your
horse foals.

The small device can be simply mounted on any horse holster.

Using a recharchable battery, WLAN and instant App notification for
Android and iOS.

[![MIT License](https://img.shields.io/badge/License-MIT-green.svg)](https://choosealicense.com/licenses/mit/)

![birth alarm device comparison between v1 and v2](content/preview.jpg?raw=true "Comparison between v1 and v2")

## Tech Stack

**Device:** 3D printed case, rechargable battery, WLAN, customizable

**Server:** Go ([Gotify](https://gotify.net/) with Docker)


## FAQ

#### What is this good for?

This device is sending an instant alarm if the horse lies down to foal.

#### Why did you do this?

I looked for a proper device that is affordable without GSM as transmission method.
We have no cell phone network in our stable, but WLAN. I did not found any products
using WLAN with simple App notification - so I build it myself.

#### How does it work?

The device is mounted on the chin strap of the horse's holster. A gyro sensor is tracking the devices position and triggers an alarm if the horse lies down.
The push notification is sent to a server, where the registered Apps are notified.


## Roadmap

- creating a DIY manual for everyone to reproduce the device

- configurable WLAN SSID and Password through Bluetooth

- smaller battery and smaller device case

- adding a measure unit to send an warning for low battery

- replacing the hardware components with smaller ones


## Feedback

If you have any feedback, please reach out to us at info@grafzahl.io


## Authors

- [@grafzahl-io](https://github.com/grafzahl-io)

