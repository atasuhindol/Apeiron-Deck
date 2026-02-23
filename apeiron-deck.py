#!/usr/bin/env python3

import serial
import time
from pystray import Icon as icon, Menu as menu, MenuItem as item
from PIL import Image, ImageDraw, ImageFont
import os
import threading
import sys

# Check the port
SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 9600
battery_level = 0

def play_notification_sound(mode_on):
    if os.name == 'nt':
        # --- WINDOWS ---
        try:
            import winsound
            sound_type = winsound.SND_ALIAS | winsound.SND_ASYNC
            if mode_on:
                winsound.PlaySound("SystemAsterisk", sound_type) # Open audio
            else:
                winsound.PlaySound("SystemHand", sound_type)     # Close audio
        except:
            pass
    else:
        # --- LINUX (CachyOS/Arch) ---
        # Genelde /usr/share/sounds altında standart sesler bulunur.
        # Mod Open audio (Tiz/Olumlu)
        on_sound = "/usr/share/sounds/freedesktop/stereo/complete.oga"
        # Mod Close audio (Tok/Uyarı)
        off_sound = "/usr/share/sounds/freedesktop/stereo/dialog-warning.oga"

        target_sound = on_sound if mode_on else off_sound

        # Arka planda çalması için komut gönderiyoruz
        # KDE/Gnome sistemlerde 'paplay' (PulseAudio) standarttır.
        if os.path.exists("/usr/bin/paplay"):
            os.system(f"paplay {target_sound} &")
        elif os.path.exists("/usr/bin/aplay"):
            os.system(f"aplay {target_sound} &")
        else:
            # Hiçbiri yoksa terminalden BİP sesi ver
            print('\a')
            sys.stdout.flush()

def get_font():
    # Linux fonts
    font_paths = [
        "/usr/share/fonts/noto/NotoSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
    ]
    for path in font_paths:
        if os.path.exists(path):
            try:
                return ImageFont.truetype(path, 28)
            except:
                continue
    return ImageFont.load_default()

def create_image(level):
    width = 64
    height = 64
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    dc = ImageDraw.Draw(image)

    # Color Pref
    fill_color = (0, 255, 0, 255) # Green
    if level < 20: fill_color = (255, 0, 0, 255) # Red
    elif level < 40: fill_color = (255, 165, 0, 255) # Orange

    # Battery Hallow
    box_margin = 4
    dc.rectangle((box_margin, 16, width - box_margin, 48), outline=(255,255,255,255), width=3)
    dc.rectangle((width - box_margin, 24, width - 1, 40), fill=(255,255,255,255))

    # Battery Persentage
    max_fill_width = (width - (2 * box_margin)) - 4
    current_fill_width = int(max_fill_width * (level / 100.0))

    if current_fill_width > 0:
        dc.rectangle((box_margin + 2, 18, box_margin + 2 + current_fill_width, 46), fill=fill_color)

    # Persantage Text
    text = f"{level}"
    font = get_font()
    try:
        left, top, right, bottom = dc.textbbox((0, 0), text, font=font)
        text_width = right - left
        text_height = bottom - top
    except:
        text_width, text_height = dc.textsize(text, font=font)

    text_x = (width - text_width) / 2
    text_y = (height - text_height) / 2 - 2

    dc.text((text_x, text_y), text, font=font, fill=(255, 255, 255, 255), stroke_width=2, stroke_fill=(0,0,0,255))
    return image

def read_serial(icon):
    global battery_level
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()

            if line.startswith("BAT:"):
                try:
                    val_str = line.split(":")[1]
                    new_level = int(val_str)
                    if new_level != battery_level:
                        battery_level = new_level
                        icon.icon = create_image(battery_level)
                        icon.title = f"Pil: %{battery_level}"
                except: pass

            elif line.startswith("SYS:"):
                cmd = line.split(":")[1]
                if cmd == "MOUSE_ON":
                    play_notification_sound(False)
                    # Linux'ta bildirim baloncuğu (opsiyonel)
                    try: icon.notify("Joystick Mouse Moduna Geçti", "Mouse Aktif")
                    except: pass
                elif cmd == "MOUSE_OFF":
                    play_notification_sound(False)
                    try: icon.notify("Oyun Kontrolcüsü Modu", "Gamepad Aktif")
                    except: pass

            time.sleep(0.1)
    except Exception as e:
        print(f"Seri Port Hatası: {e}")

def setup(icon):
    icon.visible = True
    threading.Thread(target=read_serial, args=(icon,), daemon=True).start()

# Başlat
icon('LatteDeck', create_image(0), menu=menu(item('Kapat', lambda i: i.stop()))).run(setup)
