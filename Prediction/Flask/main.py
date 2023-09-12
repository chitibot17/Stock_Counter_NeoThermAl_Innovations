from flask import Flask, render_template, request, redirect, url_for
import tensorflow as tf
import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
import random

global data

new_data = [[0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0]]
app = Flask(__name__)

@app.route('/')
def home():
    return render_template('login.html')


@app.route('/login', methods=['POST'])
def login():
    error = None
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']

        # Check if the username and password are correct
        if username != 'example' or password != 'password':
            error = 'Invalid credentials. Please try again.'
        else:
            return redirect(url_for('dashboard'))

    return render_template('login.html', error=error)


@app.route('/success')
def success():
    return 'Logged in successfully!'


@app.route('/dashboard', methods=['POST', 'GET'])
def dashboard():
    global data, model, new_data, scaler
    error = None
    if request.method == 'POST':
        name = request.form['device']
        if name == 'AC':
            pass
        if name == 'Refrigerator':
            pass
        if name == 'Heater':
            pass
        if name == 'Compressor':
            pass
        if name == 'Dehumidifiers':
            pass
        print(name)
        return redirect(url_for('model'))
    return render_template('dashboard.html', error=error)


@app.route('/model', methods=['POST', 'GET'])
def model():
    global new_data, scaler, data
    data = pd.read_csv('Bookref.csv', encoding='ISO-8859-1')
    model = tf.keras.models.load_model('ref.h5')
    new_data = np.array([[4.1, 4.2, 210, 0.4, 1100, 7, 4.7, 6]])
    temp, vib, volt, current, press = new_data[0][0:5]
    error = None
    if request.method == 'POST':
        num1 = request.form['num1']
        num2 = request.form['num2']
        if num1 != '' and num2 != '':
            new_data[0][5] = int(num1)
            new_data[0][6] = float(num2)
            scaler = StandardScaler()
            data.iloc[:, :-1] = scaler.fit_transform(data.iloc[:, :-1])
            new_data = scaler.transform(new_data)
            result = int(model.predict(new_data)[0][0])
            return render_template('model.html', temp= temp, vibration= vib, volt= volt, current=current , pressure=press , num1=num1, num2=num2, result=result)
    return render_template('model.html')


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
