
import eventlet
import json
from flask import Flask, render_template, request
from flask_mqtt import Mqtt
from flask_socketio import SocketIO
from flask_bootstrap import Bootstrap
from flask_mysqldb import MySQL
import pymysql
import datetime
eventlet.monkey_patch()

app = Flask(__name__)

app.config['SECRET'] = 'my secret key'
app.config['TEMPLATES_AUTO_RELOAD'] = True
app.config['MQTT_BROKER_URL'] = 'mqtt.eclipseprojects.io'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 5
app.config['MQTT_TLS_ENABLED'] = False
app.config['MQTT_CLEAN_SESSION'] = True

app.config['MYSQL_HOST'] = 'localhost'
app.config['MYSQL_USER'] = 'root'
app.config['MYSQL_PASSWORD'] = ''
app.config['MYSQL_DB'] = 'miniproject'

mysql = MySQL(app)
mqtt = Mqtt(app)
socketio = SocketIO(app)
bootstrap = Bootstrap(app)
data_temp = 0
data_hum =0
data_rain =0
datetime = datetime.datetime.now()
@mqtt.on_connect()
def handle_connect_hum(client, userdata, flags, rc):
    mqtt.subscribe('test')
    print('MQTT BROKER CONNECTED')

@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    global data_temp,data_hum, data_rain
    topic=message.topic
    m_decode=str(message.payload.decode("utf-8","ignore"))
    print("data Received type",type(m_decode))
    print("data Received",m_decode)
    print("Converting from Json to Object")
    m_in=json.loads(m_decode) #decode json data
    data_temp=m_in["Temp"]
    data_hum =m_in["Hum"]
    data_rain =m_in["Rain"]




@app.route('/',methods=['GET', 'POST'])
def index():

    print(request.method)
    if request.method == 'POST':
        if request.form.get('TEMP') == 'TEMP':
            # pass
            print("TEMP")
            print(request.method)
            # mqtt.publish('hum', 'this is my message ON')
            mqtt.subscribe('test')

            # mqtt.subscribe('hum')
            # mqtt.unsubscribe('temp')
            return render_template("index.html",data_hum=data_hum)
        elif  request.form.get('UPDATE') == 'UPDATE':
            # pass # do something else
            print("HUM")
            # mqtt.publish('hum', 'this is my message OFF')
            mqtt.subscribe('test')
            cursor = mysql.connection.cursor()
            cursor.execute("""INSERT INTO miniproject3 VALUES(%s,%s,%s,%s)""",(data_temp,data_hum,data_rain,datetime,))
            mysql.connection.commit()
            cursor.close()

            # mqtt.subscribe('temp')
            # mqtt.unsubscribe('hum')
            return render_template("index.html", data_temp=data_temp,data_hum=data_hum, data_rain=data_rain)
        else:
            # pass # unknown
            return render_template("index.html")
    elif request.method == 'GET':
        print(request.method)
        print("No Post Back Call")
        mqtt.subscribe('test')
        return render_template("index.html")
        

if __name__ == '__main__':
    app.run()
