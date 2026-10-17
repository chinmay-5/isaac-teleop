import socket

HOST = "0.0.0.0"
PORT = 5005
BUFFER_SIZE = 1024


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        sock.bind((HOST, PORT))
        print(f"Listening for IMU data on {HOST}:{PORT} (Ctrl+C to exit)")

        while True:
            data, addr = sock.recvfrom(BUFFER_SIZE)
            message = data.decode("utf-8", errors="replace").strip()
            print(f"{addr[0]}:{addr[1]}  {message}", flush=True)

    except KeyboardInterrupt:
        print("\nReceiver stopped.")

    finally:
        sock.close()
        print("Socket closed. Exiting.")


if __name__ == "__main__":
    main()