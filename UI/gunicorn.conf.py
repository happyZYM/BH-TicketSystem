import os
import subprocess
import signal
import atexit
from flask import Flask, render_template
from dotenv import load_dotenv
import socket

# Load environment variables from .env file
load_dotenv()

# Get configuration from environment variables
backend_ip = os.getenv('BACKEND_IP')
backend_port = os.getenv('BACKEND_PORT')
backend_log_level = os.getenv('BACKEND_LOG_LEVEL')
backend_data_directory = os.getenv('BACKEND_DATA_DIRECTORY')
backend_log_file = os.getenv('BACKEND_LOG_FILE')
backend_path = os.getenv('BACKEND_PATH')

# Command to start the backend process
backend_command = [
    backend_path,
    "-d", backend_data_directory,
    "-l", backend_log_file,
    "--level", backend_log_level,
    "server",
    "--port", backend_port,
    "--address", backend_ip
]

backend_process = None

def start_backend():
    global backend_process
    # Try to acquire the lock without blocking, timeout if another process holds the lock
    print("Starting backend process...")
    backend_process = subprocess.Popen(backend_command, preexec_fn = os.setsid)
    print(f"Backend process started with PID: {backend_process.pid}")
def on_starting(server):
    # 这里放置启动后台进程的逻辑
    start_backend()

def when_ready(server):
    print("Server is ready. Spawning workers")
def send_exit_command(ip, port):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((ip, int(port)))
            sock.sendall(b'exit\n')
            response = sock.recv(1024)  # Optional: read response from server
            print(f"Received response: {response.decode('utf-8')}")
    except Exception as e:
        print(f"Failed to send exit command: {e}")

def stop_backend():
    global backend_process
    if backend_process is not None:
        print(f"Sending exit command to backend process at {backend_ip}:{backend_port}...")
        send_exit_command(backend_ip, backend_port)
        print(f"Waiting for backend process with PID: {backend_process.pid} to terminate...")
        backend_process.wait()
        print("Backend process stopped.")
        backend_process = None

def on_exit(server):
    stop_backend()