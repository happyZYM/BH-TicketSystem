import os
import subprocess
import signal
import atexit
from flask import Flask, render_template
from flask import request, redirect, url_for, flash
from flask import session
from dotenv import load_dotenv
import time
import socket
import utils

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
app.secret_key = os.getenv('APP_SECRET_KEY')

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
    print("sending command: ", command)
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

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        
        # 调用 ExecCommand 执行登录命令
        login_response = ExecCommand(f'login -u {username} -p {password}')
        
        if login_response.strip() == '0':
            session['username'] = username  # 在session中记录用户名
            return redirect(url_for('home'))
        elif login_response.strip() == '-1':
            error = 'Invalid username or password.'
        else:
            error = 'An unknown error occurred.'
        
        return render_template('login.html', error=error)
    
    return render_template('login.html')

@app.before_request
def before_request():
    # Skip login check for login page, callback URL, and static files
    if request.endpoint in ['login', 'logout']:
        return
    # Check if user is logged in
    if ('username' not in session) or ('username' in session and utils.GetPrivilege(session['username'], ExecCommand) < 0):
        if 'username' in session:
            session.pop('username', None)
        return redirect(url_for('login'))

@app.route('/profile')
def profile():
    username = session['username']
    dat = ExecCommand(f'query_profile -c {username} -u {username}')
    print("dat: ", dat)
    # dat is something like: <username> <name> <mail> <privilege>
    profile_data = dat.split(' ')
    
    if len(profile_data) >= 4:
        user_info = {
            'username': profile_data[0],
            'name': profile_data[1],
            'email': profile_data[2],
            'privilege': profile_data[3]
        }
        return render_template('profile.html', user_info=user_info)
    else:
        # Handle the case where the response does not contain the expected number of elements
        error = "Profile data is incomplete or incorrect."
        return render_template('profile.html', error=error)

@app.route('/logout')
def logout():
    ExecCommand(f'logout -u {session["username"]}')
    session.pop('username', None)  # 从session中移除用户名
    return redirect(url_for('home'))
if __name__ == '__main__':
    app.run(debug=True)
