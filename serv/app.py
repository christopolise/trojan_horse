# Create simple flask server

from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
import json
from manuf import manuf

pars = manuf.MacParser(update=True)

app = Flask(__name__)
CORS(app)

packets = []

class Packet:
    def __init__(self, addr1, addr2, payload, pkt_t):
        self.addr1 = addr1
        self.addr2 = addr2
        self.payload = payload
        self.pkt_t = pkt_t

        self.payload = self.payload.split()
        self.payload = [int(byte, 16) for byte in self.payload]
        self.payload = [chr(byte) if byte >= 32 and byte <= 126 else '.' for byte in self.payload]
        self.payload = ''.join(self.payload)

    def to_dict(self):
        return {
            'addr1': (self.addr1, pars.get_manuf(self.addr1)),
            'addr2': (self.addr2, pars.get_manuf(self.addr2)),
            'payload': self.payload,
            'type': self.pkt_t,
        }

    @staticmethod
    def from_dict(data):
        return Packet(data['addr1'], data['addr2'], data['payload'], data['type'])

app = Flask(__name__)
CORS(app)

@app.route('/add', methods=['POST'])
def add():
    global packets
    # get addr1 addr2 addr3 from request and add them to db
    data = request.get_json()

    print(data)

    for pkt in data:
        print(pkt)

        addr1 = formatted_mac_address = ':'.join([octet.zfill(2) for octet in pkt['addr1'].split(':')])
        addr2 = formatted_mac_address = ':'.join([octet.zfill(2) for octet in pkt['addr2'].split(':')])
        payload = formatted_mac_address = pkt['payload']
        pkt_t = pkt['type']

        print(payload)

        packet = Packet(addr1, addr2, payload, pkt_t)
        packets.append(packet)

    return jsonify({"message": "Packet added successfully"})

@app.route('/sniffer_data')
def get_sniffer_data():
    global packets
    print(len(packets))
    packet_dict_list = [ pkt.to_dict() for pkt in packets ]
    return jsonify(packet_dict_list)


@app.route('/', methods=['GET'])
def index():
    return render_template('index.html')


if __name__ == '__main__':
    app.run(host='192.168.0.196', port=5000)
