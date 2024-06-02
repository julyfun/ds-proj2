# author: Benjamin2406ly
# datetime: 2024/6/1

import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from dash import Dash, html, dcc, Input, Output
import os
# print("Current working directory:", os.getcwd())


external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']
app = Dash(__name__, external_stylesheets=external_stylesheets)
# app = Dash()

# colors = {
#     'background': '#111111',
#     'text': '#7FDBFF'
# }  
event_list = pd.read_csv("./build/output.txt", skiprows=3, sep=']', header=None, names=["time", "event"])
event_list = event_list[event_list['time'].str.startswith('[')].applymap(lambda x: x.lstrip('[')).iloc[:-3]


print(event_list)
num_pack_in_station = pd.read_csv("./build/number_package_in_station.csv", sep=',', header=None, names=["time", "station", "num_pack_in_buffer"])

position = pd.read_csv("positions.csv", sep=',', header=None, names=["station", "position_x", "position_y"])

pos_and_num = pd.merge(position, num_pack_in_station, on="station")

app.layout = html.Div([
    dcc.Graph(id='graph-with-slider'),
    dcc.Slider(
        id='time-slider',
        min=pos_and_num['time'].min(),
        max=pos_and_num['time'].max(),
        value=pos_and_num['time'].min(),
        marks={str(hour): str(hour) for hour in pos_and_num['time'].unique()},
        step=None
    )
])


@app.callback(
    Output('graph-with-slider', 'figure'),
    [Input('time-slider', 'value')]
)


def update_figure(selected_time):
    filtered_df = pos_and_num[pos_and_num['time'] == selected_time]

    fig = go.Figure()

    fig.add_trace(go.Scatter(
        x=filtered_df['position_x'],
        y=filtered_df['position_y'],
        mode='markers',
        marker=dict(
            # size=1,
            color='LightSkyBlue',
            opacity=0.5,
        ),
        hoverinfo='text',
        hovertext=filtered_df['station'],
    ))
    fig.update_traces(marker=dict(size=filtered_df['num_pack_in_buffer']))

    fig.update_layout(transition_duration=500)  # 过渡时间
    return fig


if __name__ == '__main__':
    app.run_server(debug=True)

