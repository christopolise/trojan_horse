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
    def __init__(self, addr1, addr2, addr3, pkt_t):
        self.addr1 = addr1
        self.addr2 = addr2
        self.addr3 = addr3
        self.pkt_t = pkt_t

    def to_dict(self):
        return {
            'addr1': (self.addr1, pars.get_manuf(self.addr1)),
            'addr2': (self.addr2, pars.get_manuf(self.addr2)),
            'addr3': (self.addr3, pars.get_manuf(self.addr3)),
            'type': self.pkt_t,
        }

    @staticmethod
    def from_dict(data):
        return Packet(data['addr1'], data['addr2'], data['addr3'], data['type'])

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
        addr3 = formatted_mac_address = ':'.join([octet.zfill(2) for octet in pkt['addr3'].split(':')])
        pkt_t = pkt['type']

        packet = Packet(addr1, addr2, addr3, pkt_t)
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
    app.run(host='0.0.0.0', port=5000)
