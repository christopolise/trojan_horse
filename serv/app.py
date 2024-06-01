# Create simple flask server

from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
import json

app = Flask(__name__)
CORS(app)

packets = dict()

class Packet:
    def __init__(self, addr1, addr2, addr3):
        self.addr1 = addr1
        self.addr2 = addr2
        self.addr3 = addr3

    def to_dict(self):
        return {
            'addr1': self.addr1,
            'addr2': self.addr2,
            'addr3': self.addr3
        }

    @staticmethod
    def from_dict(data):
        return Packet(data['addr1'], data['addr2'], data['addr3'])

app = Flask(__name__)
CORS(app)

@app.route('/add', methods=['POST'])
def add():
    # get addr1 addr2 addr3 from request and add them to db
    data = request.get_json()

    print(data)

    addr1 = data['addr1']
    addr2 = data['addr2']
    addr3 = data['addr3']

    packet = Packet(addr1, addr2, addr3)

    # Create a new list if the key doesn't exist
    if addr1 not in packets:
        packets[addr1] = []
    packets[addr1].append(packet)

    return jsonify({"message": "Packet added successfully"})


@app.route('/', methods=['GET'])
def index():
    return render_template('index.html')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
