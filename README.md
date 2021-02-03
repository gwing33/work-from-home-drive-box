# Work From Home Drive Box

This was built for Mark Rober's Monthly class (Jan 2021). It was the final project build.

First time doing any Arduino coding, so it's very rough around the edges. Both in the code and for the electrical.

## Server Dependencies

This project requires a server, and that project is [located over here](https://github.com/gwing33/home-harness)

## iOS Setup

Right now I used the Shortcut app which allows you to make some automations.

- When leave the house, make an HTTPS GET request to `{yourPublicUrl}/wfh/leave`
- Wait to return home
- When return the house, make an HTTPS GET request to `{yourPublicUrl}/wfh/return`

## Wiring and Electronics

See the images/\* for how I wired everything. What I used for this project:

- Arduino Uno
- PIR Sensor
- Servo
- ESP8266 Wifi Module

## Known issues / limitations

- This is all run off the Arduino Uno power, which I know is not ideal.
- Anytime the ESP8266 does anything, the servo twitches. I was unable to resolve that issue.
- iOS shortcuts app doesn't currently run the automation, which is sort of a bummer, it presents you with a notification, and a button to press "run".
