import collections
import socket
import threading
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Network configuration
UDP_IP = "0.0.0.0"
UDP_PORT = 5005
BUFFER_SIZE = 50  # Number of historical data points to keep in the plot window

# Thread-safe ring buffers for timestamps and IMU data
timestamps = collections.deque(maxlen=BUFFER_SIZE)
acc_x = collections.deque(maxlen=BUFFER_SIZE)
acc_y = collections.deque(maxlen=BUFFER_SIZE)
acc_z = collections.deque(maxlen=BUFFER_SIZE)
gyr_x = collections.deque(maxlen=BUFFER_SIZE)
gyr_y = collections.deque(maxlen=BUFFER_SIZE)
gyr_z = collections.deque(maxlen=BUFFER_SIZE)

data_lock = threading.Lock()
stop_event = threading.Event()


def udp_listener():
    """Background thread to receive and parse incoming IMU UDP packets."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((UDP_IP, UDP_PORT))
    sock.settimeout(0.5)

    print(f"Listening for IMU UDP packets on {UDP_IP}:{UDP_PORT}...")
    start_time_ticks = None

    while not stop_event.is_set():
        try:
            raw_data, _ = sock.recvfrom(1024)
            message = raw_data.decode("utf-8").strip()

            # Packet format: timestamp_ticks,acc_x,acc_y,acc_z,gyr_x,gyr_y,gyr_z
            parts = [float(x) for x in message.split(",") if x.strip()]
            if len(parts) == 7:
                raw_time, ax, ay, az, gx, gy, gz = parts
                if start_time_ticks is None:
                    start_time_ticks = raw_time

                # Convert ticks (assuming 1 tick = 10ms or relative time offset)
                t_rel = (raw_time - start_time_ticks) / 100.0  # seconds relative

                with data_lock:
                    timestamps.append(t_rel)
                    acc_x.append(ax)
                    acc_y.append(ay)
                    acc_z.append(az)
                    gyr_x.append(gx)
                    gyr_y.append(gy)
                    gyr_z.append(gz)

        except socket.timeout:
            continue
        except (ValueError, UnicodeDecodeError):
            continue
        except Exception as e:
            if not stop_event.is_set():
                print(f"Error receiving data: {e}")
            break

    sock.close()
    print("UDP listener stopped.")

def main():
    # Start the UDP receiver thread
    listener_thread = threading.Thread(target=udp_listener, daemon=True)
    listener_thread.start()

    # Set up matplotlib figure and subplots
    fig, (ax_acc, ax_gyr) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
    fig.canvas.manager.set_window_title("IMU Real-Time Telemetry")

    # Accelerometer lines
    (line_ax,) = ax_acc.plot([], [], label="Acc X", color="#e74c3c", lw=1.5)
    (line_ay,) = ax_acc.plot([], [], label="Acc Y", color="#2ecc71", lw=1.5)
    (line_az,) = ax_acc.plot([], [], label="Acc Z", color="#3498db", lw=1.5)
    ax_acc.set_title("Accelerometer Data (mg)", fontsize=12, fontweight="bold")
    ax_acc.set_ylabel("Acceleration (mg)")
    ax_acc.grid(True, linestyle="--", alpha=0.6)
    ax_acc.legend(loc="upper right")

    # Gyroscope lines
    (line_gx,) = ax_gyr.plot([], [], label="Gyr X", color="#e67e22", lw=1.5)
    (line_gy,) = ax_gyr.plot([], [], label="Gyr Y", color="#9b59b6", lw=1.5)
    (line_gz,) = ax_gyr.plot([], [], label="Gyr Z", color="#1abc9c", lw=1.5)
    ax_gyr.set_title("Gyroscope Data (mdps)", fontsize=12, fontweight="bold")
    ax_gyr.set_ylabel("Angular Rate (mdps)")
    ax_gyr.set_xlabel("Relative Time (s)")
    ax_gyr.grid(True, linestyle="--", alpha=0.6)
    ax_gyr.legend(loc="upper right")

    def update(_frame):
        with data_lock:
            if not timestamps:
                return line_ax, line_ay, line_az, line_gx, line_gy, line_gz

            t = list(timestamps)
            ax_data = list(acc_x)
            ay_data = list(acc_y)
            az_data = list(acc_z)
            gx_data = list(gyr_x)
            gy_data = list(gyr_y)
            gz_data = list(gyr_z)

        # Update accelerometer data
        line_ax.set_data(t, ax_data)
        line_ay.set_data(t, ay_data)
        line_az.set_data(t, az_data)

        # Update gyroscope data
        line_gx.set_data(t, gx_data)
        line_gy.set_data(t, gy_data)
        line_gz.set_data(t, gz_data)

        # Update x-axis limits (sliding window)
        t_min, t_max = t[0], t[-1]
        if t_min == t_max:
            t_max += 0.1
        ax_acc.set_xlim(t_min, t_max)
        ax_gyr.set_xlim(t_min, t_max)

        # Rescale y-axes to fit incoming values with margins
        acc_vals = ax_data + ay_data + az_data
        if acc_vals:
            min_acc, max_acc = min(acc_vals), max(acc_vals)
            pad_acc = max((max_acc - min_acc) * 0.1, 50.0)
            ax_acc.set_ylim(min_acc - pad_acc, max_acc + pad_acc)

        gyr_vals = gx_data + gy_data + gz_data
        if gyr_vals:
            min_gyr, max_gyr = min(gyr_vals), max(gyr_vals)
            pad_gyr = max((max_gyr - min_gyr) * 0.1, 50.0)
            ax_gyr.set_ylim(min_gyr - pad_gyr, max_gyr + pad_gyr)

        return line_ax, line_ay, line_az, line_gx, line_gy, line_gz

    ani = animation.FuncAnimation(fig, update, interval=40, blit=False, cache_frame_data=False)

    def on_close(event):
        stop_event.set()

    fig.canvas.mpl_connect("close_event", on_close)

    try:
        plt.tight_layout()
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        listener_thread.join(timeout=1.0)
        print("Exiting.")


if __name__ == "__main__":
    main()