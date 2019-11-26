import SwiftSerial
import PythonKit
import Foundation

let samplesPerSecond = 5000
let seconds = 3
let samples = samplesPerSecond * seconds
let port = "/dev/cu.usbmodem0E2082DE1"

let plotter = Python.import("matplotlib.pyplot")

let x: [Int] = Array<Int>(0..<samples)
var y: [Int] = []

y.reserveCapacity(samples)

let serialPort = SerialPort(path: port)

do {
    print("Opening \(port)…")
    try serialPort.openPort()
    print("Opened successfully.")
    defer {
        serialPort.closePort()
        print("Port Closed")
    }
    
    serialPort.setSettings(
        receiveRate: .baud115200,
        transmitRate: .baud115200,
        minimumBytesToRead: 1
    )

    print("Reading samples…")
    for i in 0..<samples {
        let stringReceived = try serialPort.readUntilChar(10)
        guard let intReceived = Int(stringReceived.trimmingCharacters(
            in: .whitespacesAndNewlines
        )) else {
            print("Invalid value received.")
            exit(65)
        }
        y.append(intReceived)
        print("\(i)/\(samples)", terminator: "\r")
    }
    print("Done.")

    let x_labels = x.map { value in Double(value) / Double(samplesPerSecond) }
    let plot = plotter.plot(x_labels,y)
    plotter.title("Cardiogram")
    plotter.setp(plot, color: "r", linewidth: 0.5)
    plotter.show()
}
