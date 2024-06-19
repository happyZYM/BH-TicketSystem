import os
import subprocess
import signal
import atexit
from flask import Flask, render_template
from dotenv import load_dotenv
import time
import socket

# Load environment variables from .env file
load_dotenv()

app = Flask(__name__)

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

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((backend_ip, int(backend_port)))
def ExecCommand(command):
    command=command+'\n'
    sock.sendall(command.encode('utf-8'))
    response = sock.recv(1024*1024)
    response= response.decode('utf-8')
    _, _, response = response.partition(" ")
    return response

# Signal handler for graceful shutdown
def signal_handler(signum, frame):
    # stop_backend()
    time.sleep(10)
    exit(0)

# Register signal handlers
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/about')
def about():
    return render_template('about.html')

if __name__ == '__main__':
    app.run(debug=True)
