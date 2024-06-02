# author: Benjamin2406ly
# datetime: 2024/6/1

import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from dash import Dash, html, dcc, Input, Output
import os
# print("Current working directory:", os.getcwd())

# CSS
external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']
app = Dash(__name__, external_stylesheets=external_stylesheets)


# data process
event_list = pd.read_csv("./build/output.txt", skiprows=3, sep=']', header=None, names=["time", "station", "event"])
event_list = event_list[event_list['time'].str.startswith('[')].iloc[:-3]
event_list['time'] = event_list['time'].apply(lambda x: x.lstrip('['))

station = event_list['station']
times = event_list['time']

num_pack_in_station = pd.read_csv("./build/number_package_in_station.csv", sep=',', header=None, names=["time", "station", "num_pack_in_buffer"])

position = pd.read_csv("positions.csv", sep=',', header=None, names=["station", "position_x", "position_y"])

pos_and_num = pd.merge(position, num_pack_in_station, on="station")

routes = pd.read_csv("routes.csv", sep=',', header=None, names=["src", "dst"])

# UI
app.layout = html.Div([
    dcc.Graph(id='graph-with-slider'),
    dcc.Slider(
        id='time-slider',
        min=pos_and_num['time'].min(),
        max=pos_and_num['time'].max(),
        value_slider=pos_and_num['time'].min(),
        marks={str(hour): str(hour) for hour in pos_and_num['time'].unique()},
        step=None
    ),

    html.Label('Select one station to check'),
    dcc.Dropdown(
        options=[
            {"label": s, "value": s} for s in pos_and_num['station'].unique()
        ],
        value='s0',
    ),
    html.Br(),
])

@app.callback(
    Output('graph-with-slider', 'figure'),
    Input('time-slider', 'value_slider')
)

def update_figure(selected_time):
    filtered_df = pos_and_num[pos_and_num['time'] == selected_time]

    fig = go.Figure()

    fig.add_trace(go.Scatter(
        x=filtered_df['position_x'],
        y=filtered_df['position_y'],
        mode='markers',
        marker=dict(
            colorscale='Viridis',
            cmin=0,
            cmax=100,
            colorbar=dict(title='Number of packages in buffer',
                          tickvals=[0,25,50,75,100],
                          ticktext=['0','25','50','75','100'],),
            opacity=0.5,
        ),
        hoverinfo='text',
        hovertext=filtered_df['station'],
    ))
    fig.update_traces(marker=dict(size=filtered_df['num_pack_in_buffer'],
                                  color=filtered_df['num_pack_in_buffer'],))
    
    for index, row in routes.iterrows():
        src_type = row['src'][0]
        dst_type = row['dst'][0]
        x_coords = [filtered_df.loc[filtered_df['station'] == row['src']]['position_x'].iloc[0],
                    filtered_df.loc[filtered_df['station'] == row['dst']]['position_x'].iloc[0]]
        y_coords = [filtered_df.loc[filtered_df['station'] == row['src']]['position_y'].iloc[0],
                    filtered_df.loc[filtered_df['station'] == row['dst']]['position_y'].iloc[0]]
    
        if src_type=='c' and dst_type=='c':
            line_color = 'red'
            dash = 'dot'
        elif (src_type=='c' and dst_type=='s') or (src_type=='s' and dst_type=='c'):
            line_color = 'green'
            dash = 'solid'
        elif src_type=='s' and dst_type=='s':
            line_color = 'gray'
            dash = 'dash'

        fig.add_trace(go.Scatter(
            x=x_coords,
            y=y_coords,
            mode='lines',
            line=dict(color=line_color,
                      width=2,
                      dash=dash),
            hoverinfo='text',
            hovertext=(row['src'], row['dst']),
            opacity=0.2,
        ))


    fig.update_layout(transition_duration=200,
                       width=1000,
                       height=800)  
    return fig


if __name__ == '__main__':
    app.run_server(debug=True)

