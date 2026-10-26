"""
rover_console — guida il rover dal PC con il protocollo binario.

Uso:
    python tools/rover_console.py            (porta trovata in automatico)
    python tools/rover_console.py COM5       (anche la porta COM del Bluetooth)

Tasti:
    w avanti   s indietro   a ruota sx   d ruota dx   x / spazio stop
    + / -  velocità            j / l servo −10° / +10°   k servo al centro
    r rilascia servo           [ / ]  trim servo         S salva calibrazioni
    q esci

Invia DRIVE 10 volte al secondo (è anche l'heartbeat: se il programma si
chiude, dopo 500 ms il rover si ferma da solo) e mostra i frame STATUS.
"""

import msvcrt
import struct
import sys
import time

import serial
import serial.tools.list_ports

BAUD = 9600
SOF = b"\xAA\x55"

MSG_HELLO = 0x01
MSG_HELLO_ACK = 0x02
MSG_DRIVE = 0x10
MSG_SERVO = 0x11
MSG_SERVO_TRIM = 0x20
MSG_SAVE_CALIB = 0x21
MSG_STATUS = 0x30

RESET_CAUSES = ["accensione", "reset esterno", "brown-out", "WATCHDOG", "sconosciuta"]


def crc8(data: bytes) -> int:
    """CRC-8, polinomio 0x07, init 0x00 — stesso algoritmo del firmware."""
    crc = 0
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


assert crc8(b"123456789") == 0xF4


def frame(msg_id: int, payload: bytes = b"") -> bytes:
    body = bytes([msg_id, len(payload)]) + payload
    return SOF + body + bytes([crc8(body)])


class Parser:
    """Stessa macchina a stati del firmware, in Python."""

    def __init__(self):
        self.buf = bytearray()
        self.errors = 0

    def feed(self, data: bytes):
        self.buf += data
        frames = []
        while True:
            start = self.buf.find(SOF)
            if start < 0:
                del self.buf[:-1]          # tiene un eventuale 0xAA finale
                return frames
            del self.buf[:start]
            if len(self.buf) < 4:
                return frames
            msg_id, length = self.buf[2], self.buf[3]
            total = 4 + length + 1
            if len(self.buf) < total:
                return frames
            body = bytes(self.buf[2:4 + length])
            if crc8(body) == self.buf[4 + length]:
                frames.append((msg_id, body[2:]))
                del self.buf[:total]
            else:
                self.errors += 1
                del self.buf[:1]           # risincronizzazione


def find_port() -> str:
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        sys.exit("Nessuna porta seriale trovata: l'Arduino è collegato?")
    return ports[0].device


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    ser = serial.Serial(port, BAUD, timeout=0)
    print(f"Porta {port}: attendo il riavvio dell'Arduino...")
    time.sleep(2.0)                        # l'apertura della porta resetta la scheda
    ser.reset_input_buffer()
    print("In attesa di risposta dal rover (HELLO ogni secondo)...")

    parser = Parser()
    drive_l = drive_r = 0
    speed = 200
    servo_deg, servo_on = 0, True
    next_drive = 0.0
    status_line = ""
    sent_drive = 0                         # DRIVE inviati, per calcolare i persi
    first_rx = None                        # (inviati, ricevuti) al primo STATUS
    hello_ok = False
    next_hello = 0.0
    last_rx = time.monotonic()
    warned_len = set()

    def event(text):
        print("\r" + " " * len(status_line) + "\r" + text)

    while True:
        # --- tastiera
        while msvcrt.kbhit():
            key = msvcrt.getwch()
            if key == "q":
                ser.write(frame(MSG_DRIVE, struct.pack("<bbB", 0, 0, speed)))
                ser.close()
                print()
                return
            moves = {"w": (1, 1), "s": (-1, -1), "a": (-1, 1), "d": (1, -1), "x": (0, 0), " ": (0, 0)}
            if key in moves:
                drive_l, drive_r = moves[key]
            elif key in "+-":
                speed = max(0, min(255, speed + (20 if key == "+" else -20)))
            elif key in "jlk":
                servo_deg = 0 if key == "k" else max(-40, min(40, servo_deg + (10 if key == "l" else -10)))
                servo_on = True
                ser.write(frame(MSG_SERVO, struct.pack("<bB", servo_deg, 1)))
            elif key == "r":
                servo_on = False
                ser.write(frame(MSG_SERVO, struct.pack("<bB", servo_deg, 0)))
            elif key in "[]":
                ser.write(frame(MSG_SERVO_TRIM, struct.pack("<b", 1 if key == "]" else -1)))
            elif key == "S":
                ser.write(frame(MSG_SAVE_CALIB))
                event("calibrazioni salvate")

        # --- HELLO finché il rover non risponde
        now = time.monotonic()
        if not hello_ok and now >= next_hello:
            ser.write(frame(MSG_HELLO))
            next_hello = now + 1.0

        # --- heartbeat
        if now >= next_drive:
            ser.write(frame(MSG_DRIVE, struct.pack("<bbB", drive_l, drive_r, speed)))
            sent_drive += 1
            next_drive = now + 0.1

        # --- ricezione
        data = ser.read(256)
        if data:
            last_rx = now
        elif now - last_rx > 3.0:
            event("nessun byte dal rover da 3 s: controlla TXD → D0, alimentazione e baud rate")
            last_rx = now
        for msg_id, payload in parser.feed(data):
            expected = {MSG_HELLO_ACK: 4, MSG_STATUS: 21}.get(msg_id)
            if expected is not None and len(payload) != expected and msg_id not in warned_len:
                warned_len.add(msg_id)
                event(f"messaggio 0x{msg_id:02X} di {len(payload)} byte invece di {expected}: "
                      "firmware e tool non sono allineati")
            if msg_id == MSG_HELLO_ACK and len(payload) == 4:
                hello_ok = True
                major, minor, proto, cause = payload
                event(f"rover firmware {major}.{minor}, protocollo {proto}, "
                      f"reset: {RESET_CAUSES[min(cause, 4)]}")
            elif msg_id == MSG_STATUS and len(payload) == 21:
                (t, vbat, dist, flags, pwm_l, pwm_r, sdeg, trim,
                 tx_lost, perr, drive_rx) = struct.unpack("<IHHBhhbbHHH", payload)
                if first_rx is None:
                    first_rx = (sent_drive, drive_rx)
                d_sent = sent_drive - first_rx[0]
                d_rx = (drive_rx - first_rx[1]) & 0xFFFF
                loss = 100.0 * (d_sent - d_rx) / d_sent if d_sent > 20 else 0.0
                dist_txt = f"{dist:4d} mm" if flags & 0x02 else "  -- mm"
                status_line = (
                    f"t={t / 1000:7.1f}s  pile={vbat:5d}mV{'' if flags & 0x01 else ' (OFF)'}  "
                    f"dist={dist_txt}  pwm={pwm_l:4d}/{pwm_r:4d}  v={speed:3d}  "
                    f"servo={sdeg:3d}{'' if flags & 0x10 else 'R'} trim={trim:3d}  "
                    f"{'AEB ' if flags & 0x04 else ''}{'LINK' if flags & 0x08 else 'no link'}  "
                    f"err={perr}/{parser.errors} persi={tx_lost} drive persi={max(loss, 0):4.1f}%"
                )
                print("\r" + status_line, end="", flush=True)

        time.sleep(0.01)


if __name__ == "__main__":
    main()
