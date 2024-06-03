
import random
import numpy as np
import matplotlib.pyplot as plt
import uuid
from sklearn.cluster import KMeans
import os
# print("Current working directory:", os.getcwd())

parameters = {
    "station_num": 15,
    "center_num": 5,
    "packet_num": 2500,
    "money_cost_per_dist_airline": 0.2, # line 79
    "money_cost_per_dist_highway": 0.12,# line 92
    "money_cost_per_dist_road": 0.07,   # line 105
    "time_cost_per_dist_airline": 0.25, # line 79
    "time_cost_per_dist_highway": 0.6,  # line 92
    "time_cost_per_dist_road": 0.8,     # line 105
    "road_length_limit": 30,            # line 101
    "standard_ratio_express": [0.7, 0.3],  # line 120
    # station_prop at line 29
    # center_prop at line 55
}


def data_gen():
    # Generate Stations
    station_id = [f"s{i}" for i in range(parameters["station_num"])]
    station_pos = []
    # properties are defined here: throughput/tick, time_delay, money_cost
    station_prop_candidates = [
        (10, 2, 0.5), (15, 2, 0.6), (20, 1, 0.8), (25, 1, 0.9)]
    station_prop = []
    for i in range(parameters["station_num"]):
        # Map size is defined here, which is 100*100
        station_pos.append((random.randint(0, 100), random.randint(0, 100)))
        station_prop.append(
            station_prop_candidates[random.randint(0, len(station_prop_candidates)-1)])
    # Output Stations
    print("Stations:")
    for i in range(len(station_pos)):
        print(station_id[i], station_pos[i], station_prop[i])
    # Generate Centers by clustering
    kmeans = KMeans(n_clusters=parameters["center_num"])
    kmeans.fit(station_pos)
    station_labels = kmeans.predict(station_pos)
    center_id = [f"c{i}" for i in range(parameters["center_num"])]
    center_pos = [(int(x[0]), int(x[1])) for x in kmeans.cluster_centers_]
    for i in range(len(center_pos)):
        while center_pos[i] in station_pos:
            # move slightly if center is overlapped with station
            # you can also use other methods to avoid this situation
            print("Warning: Center moved")
            center_pos[i] = center_pos[i][0] + 1, center_pos[i][1] + 1
    # properties are defined here: throughput/tick, time_delay, money_cost
    center_prop_candidates = [
        (100, 2, 0.5), (150, 2, 0.5), (125, 1, 0.5), (175, 1, 0.5)]
    center_prop = []
    for i in range(parameters["center_num"]):
        center_prop.append(
            center_prop_candidates[random.randint(0, len(center_prop_candidates)-1)])
    # Output Centers
    print("Centers:")
    for i in range(parameters["center_num"]):
        print(center_id[i], center_pos[i], center_prop[i])

    # Draw Stations and Centers
    plt.scatter([x[0] for x in station_pos], [x[1]
                for x in station_pos], c=station_labels, s=50, cmap='viridis')
    plt.scatter([x[0] for x in center_pos], [x[1]
                for x in center_pos], c='black', s=200, alpha=0.5)

    # Generate Edges
    edges = []
    print("Edges (center to center):")      # Airlines
    for i in range(parameters["center_num"]):
        for j in range(parameters["center_num"]):
            if j > i:
                dist = np.linalg.norm(
                    np.array(center_pos[i]) - np.array(center_pos[j]))
                # src, dst, time_cost, money_cost
                # time_cost and money_cost are defined here
                edges.append((center_id[i], center_id[j], 0.25 * dist, 0.2 * dist))
                edges.append((center_id[j], center_id[i], 0.25 * dist, 0.2 * dist))
                plt.plot([center_pos[i][0], center_pos[j][0]], [
                         center_pos[i][1], center_pos[j][1]], 'r--')
                # print(edges[-2])
                # print(edges[-1])
    print("Edges (center to station):")     # Highways
    for i in range(parameters["center_num"]):
        for j in range(parameters["station_num"]):
            if station_labels[j] == i:
                dist = np.linalg.norm(
                    np.array(center_pos[i]) - np.array(station_pos[j]))
                # time_cost and money_cost are defined here
                edges.append((center_id[i], station_id[j], 0.6 * dist, 0.12 * dist))
                edges.append((station_id[j], center_id[i], 0.6 * dist, 0.12 * dist))
                plt.plot([center_pos[i][0], station_pos[j][0]], [
                         center_pos[i][1], station_pos[j][1]], 'b--')
                # print(edges[-2])
                # print(edges[-1])
    print("Edges (station to station):")    # Roads
    for i in range(parameters["station_num"]):
        for j in range(parameters["station_num"]):
            if i > j and (np.linalg.norm(np.array(station_pos[i]) - np.array(station_pos[j])) < 30):
                dist = np.linalg.norm(
                    np.array(station_pos[i]) - np.array(station_pos[j]))
                # time_cost and money_cost are defined here
                edges.append((station_id[i], station_id[j], 0.8 * dist, 0.07*dist))
                edges.append((station_id[j], station_id[i], 0.8 * dist, 0.07*dist))
                plt.plot([station_pos[i][0], station_pos[j][0]], [
                         station_pos[i][1], station_pos[j][1]], 'g--')
                # print(edges[-2])
                # print(edges[-1])
    # plt.show()

    # Generate Packets
    packets = []
    src_prob = np.random.random(parameters["station_num"])
    src_prob = src_prob / np.sum(src_prob)
    dst_prob = np.random.random(parameters["station_num"])
    dst_prob = dst_prob / np.sum(dst_prob)
    # Package categories are defined here: 0 for Regular, 1 for Express
    speed_prob = [0.7, 0.3]
    print("Packets:")
    for i in range(parameters["packet_num"]):      # Number of packets
        src = np.random.choice(parameters["station_num"], p=src_prob)
        dst = np.random.choice(parameters["station_num"], p=dst_prob)
        while dst == src:
            dst = np.random.choice(parameters["station_num"], p=dst_prob)
        category = np.random.choice(2, p=speed_prob)
        category_name = ["PackageCategory::STANDARD", "PackageCategory::EXPRESS"]
        # Create time of the package, during 12 time ticks(hours). Of course you can change it.
        create_time = np.random.random() * 1
        id = uuid.uuid4()
        packets.append((id, create_time, category, f"s{src}", f"s{dst}"))
    # Sort packets by create time
    packets.sort(key=lambda x: x[0])    # Sort by create time from small to large
    # Output Packets
    # for packet in packets:
    #     print(uuid.uuid4(), packet)

    return {
        "station_id": station_id,
        "station_pos": station_pos,
        "station_prop": station_prop,
        "center_id": center_id,
        "center_pos": center_pos,
        "center_prop": center_prop,
        "edges": edges,
        "packets": packets,
    }

if __name__ == '__main__':
    data = data_gen()
    with open("data.txt", "w") as f:
        f.write("stations:"+"\n")
        for i, station in enumerate(data["station_id"]):
            f.write(station+' , '+
                    # str(data["station_pos"][i])+" , "+
                    str(data["station_prop"][i])+"\n")

        # f.write("centers:"+"\n")
        for i, center in enumerate(data["center_id"]):
            f.write(center+' , '+
                    # str(data["center_pos"][i])+" , "+
                    str(data["center_prop"][i])+"\n")

        f.write("edges:"+"\n")
        for i, edge in enumerate(data["edges"]):
            f.write(str(edge[0])+" , "+
                    str(edge[1])+" , "+
                    str(edge[2])+" , "+
                    str(edge[3])+"\n")
        
        f.write("packets:"+"\n")
        for i, packet in enumerate(data["packets"]):
            f.write(str(packet[0])+" , "+
                    str(packet[1])+" , "+
                    str(packet[2])+" , "+
                    str(packet[3])+" , "+
                    str(packet[4])+"\n") 
    
    with open("positions.csv", "w") as f:
        for i, station in enumerate(data["station_id"]):
            f.write(station+','+
                    str(data["station_pos"][i][0])+','+
                    str(data["station_pos"][i][1])+"\n")
        for i, center in enumerate(data["center_id"]):
            f.write(center+','+
                    str(data["center_pos"][i][0])+','+
                    str(data["center_pos"][i][1])+"\n")
            
    with open("routes.csv", "w") as f:
        for i, edge in enumerate(data["edges"]):
            f.write(edge[0]+','+
                    edge[1]+','+
                    str(edge[2])+'\n')
    
    with open("package_ctg.csv", "w") as f:
        for i, packet in enumerate(data["packets"]):
            f.write(str(packet[0])+','+
                    str(packet[2])+'\n')
