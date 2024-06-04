# author: Benjamin2406ly
# datetime: 2024/6/1

import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from dash import Dash, html, dcc, Input, Output, State, no_update
import os
# print("Current working directory:", os.getcwd())

# CSS
external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']
app = Dash(__name__, external_stylesheets=external_stylesheets)

# data process
event_list = pd.read_csv("./build/output.txt", skiprows=3, sep=']', header=None, names=["time", "station", "event"])
event_list = event_list[event_list['time'].str.startswith('[')].iloc[:-3]
event_list['time'] = event_list['time'].apply(lambda x: x.lstrip('['))
# time_of_selected_station = pd.DataFrame(columns=['time'])
# for index, row in event_list.iterrows():
#     if row['station'].strip() == 's0':
#         time_of_selected_station.loc[len(time_of_selected_station)] = row['time']
# time_options = [{'label': i, 'value': i} for i in time_of_selected_station['time'].unique()]
# print(time_options)

num_pack_in_station = pd.read_csv("./build/number_package_in_station.csv", sep=',', header=None, names=["time", "station", "num_pack_in_buffer"])

position = pd.read_csv("positions.csv", sep=',', header=None, names=["station", "position_x", "position_y"])
pos_and_num = pd.merge(position, num_pack_in_station, on="station")

routes = pd.read_csv("routes.csv", sep=',', header=None, names=["src", "dst", "time_cost"])

package_trip = pd.read_csv("./build/package_trip.csv", sep=',', header=None, names=["time", "package_id", "src", "dst"])
# package_trip_new = pd.merge(package_trip, routes, on=["src", "dst"])

package_ctg = pd.read_csv("package_ctg.csv", sep=',', header=None, names=["package_id", "category"])

# Layout
app.layout = html.Div([
    html.H1(
        children="Package Distribution System by 后排靠窗四高手",
        style={
            'textAlign': 'center',
            'color': '#111111'
        }),
    dcc.Graph(id='graph-with-slider'),
    dcc.Slider(
        id='time-slider',
        min=pos_and_num['time'].min(),
        max=pos_and_num['time'].max(),
        value=pos_and_num['time'].min(),
        marks={str(hour): str(hour) for hour in pos_and_num['time'].unique()},
        step=None
    ),

    html.Div([
        html.Div([
            html.Label('Select one with station to check'),
            dcc.Dropdown(
                id='station-dropdown',
                options=[
                    {"label": s, "value": s} for s in pos_and_num['station'].unique()
                ],
                value='s0',
            ),
        ], style={'width': '48%', 'display': 'inline-block'}),
        html.Div([
            html.Label('Select one with time to check'),
            dcc.Dropdown(
                id='time-dropdown',
                options=[],
                value=None,
            ),
        ], style={'width': '48%', 'display': 'inline-block'}),
    ]),

    html.Div(id='check_output',
             style={'margin-top': 20, 'background-color': 'lightgray', 'padding': '10px'}),

    html.Div([
        html.Label('Select one with package id to check package in Graph'),
        dcc.Dropdown(
            id='package-id-dropdown-graph',
            options=[
                {"label": s, "value": s} for s in package_trip['package_id'].unique()
            ],
            value=package_trip['package_id'].iloc[0],
        ),
    ], style={'margin-top': 20}),

    dcc.Graph(id='animation-with-events'),
    # html.Div([
    #     html.Button('Start/Stop', id='start-stop-button', n_clicks=0, style={'margin-top': 10}),   
    # ], style={'textAlign': 'center'}),
    
    
    # dcc.Interval(
    #     id='interval-component',
    #     interval=1*100,
    #     n_intervals=0
    # ),

    html.Div([
        html.Label('Select one with package id to check package'),
        dcc.Dropdown(
            id='package-id-dropdown',
            options=[
                {"label": s, "value": s} for s in package_trip['package_id'].unique()
            ],
            value=package_trip['package_id'].iloc[0],
        ),
    ]),

    html.Div(id='check_pacakge',
             style={'margin-top': 20, 'background-color': 'lightgray', 'padding': '10px'}),
])

# Callbacks
@app.callback(
    Output('graph-with-slider', 'figure'),
    [Input('time-slider', 'value'),]
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
        hovertext=filtered_df['station']+" "+"number of packages in buffer: "+filtered_df['num_pack_in_buffer'].astype(str),
        showlegend=False,
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
            hoverinfo='none',
            showlegend=False,
            opacity=0.2,
        ))

    fig.update_layout(transition_duration=200,
                       width=1000,
                       height=800)  
    return fig

@app.callback(
    Output('time-dropdown', 'options'),
    Input('station-dropdown', 'value'),
)
def update_time_options(selected_station):
    time_of_selected_station = pd.DataFrame(columns=['time'])
    for index, row in event_list.iterrows():
        if row['station'].strip() == selected_station:
            time_of_selected_station.loc[len(time_of_selected_station)] = row['time']
    time_options = [{'label': i, 'value': i} for i in time_of_selected_station['time'].unique()]
    return time_options

@app.callback(
    Output('check_output', 'children'),
    Input('station-dropdown', 'value'),
    Input('time-dropdown', 'value'),
)

def update_check_output(selected_station, selected_time):
    events = []
    for index, row in event_list.iterrows():
        if row['station'].strip() == selected_station and row['time'].strip() == selected_time:
            events.append(row['event']+" ")
    
    return f"You selected Station {selected_station} at Time {selected_time}, the event is: \n{''.join(events)}"

@app.callback(
    Output('animation-with-events', 'figure'),
    [Input('package-id-dropdown-graph', 'value'),],
    # [Input('interval-component', 'n_intervals')],
    # [State('start-stop-button', 'n_clicks')]
)

def update_animation(selected_package_id):   
    # global update_graph

    # if click % 2 == 0:
    #     update_graph = False
    # else:
    #     update_graph = True
    
    # if not update_graph:
    #     return no_update
    
    # lastest_data = package_trip_new[package_trip_new['time'].astype(float) <= n]
    # lastest_data = lastest_data.tail(5)

    filtered_package = package_trip[package_trip['package_id'].str.strip() == selected_package_id]

    fig = go.Figure()
    s = go.Scatter(x=pos_and_num['position_x'], 
                   y=pos_and_num['position_y'], 
                   marker=dict(color='blue', 
                               size=10),
                   hovertext=pos_and_num['station'],
                   mode='markers',
                   showlegend=False,
                   opacity=0.5)
    fig.add_trace(s)

    # for index, trip in lastest_data.iterrows():
    #     src_x = position.loc[position['station']==trip['src']]['position_x'].values[0]
    #     src_y = position.loc[position['station']==trip['src']]['position_y'].values[0]
    #     dst_x = position.loc[position['station']==trip['dst']]['position_x'].values[0]
    #     dst_y = position.loc[position['station']==trip['dst']]['position_y'].values[0]
    #     ctg = package_ctg.loc[package_ctg['package_id']==trip['package_id']]['category'].values[0]
    #     if trip['src']==trip['dst']:   
    #         package = go.Scatter(x=[src_x], 
    #                              y=[src_y], 
    #                              mode='markers+text',
    #                              marker=dict(color='red' if ctg==1 else 'green', 
    #                                          size=10),
    #                              text=trip['package_id'][:4],
    #                              name=trip['package_id'],
    #                              showlegend=False)
    #     else:
    #         package_x=(dst_x-src_x)*min(((n/10-float(trip['time']))/float(trip['time_cost'])),1)+src_x
    #         package_y=(dst_y-src_y)*min(((n/10-float(trip['time']))/float(trip['time_cost'])),1)+src_y
    #         package = go.Scatter(x=[package_x],
    #                             y=[package_y],
    #                             mode='markers+text',
    #                             marker=dict(color='red' if ctg==1 else 'green', 
    #                                         size=10),
    #                             text=trip['package_id'][:4],
    #                             name=trip['package_id'],
    #                             showlegend=False)
    #         fig.add_trace(package)
    for index, trip in filtered_package.iterrows():
        if trip['src'] != trip['dst']:
            src_x = position.loc[position['station']==trip['src']]['position_x'].values[0]
            src_y = position.loc[position['station']==trip['src']]['position_y'].values[0]
            dst_x = position.loc[position['station']==trip['dst']]['position_x'].values[0]
            dst_y = position.loc[position['station']==trip['dst']]['position_y'].values[0]
            ctg = package_ctg.loc[package_ctg['package_id']==trip['package_id']]['category'].values[0]

            trace = go.Scatter(
                x=[src_x, dst_x],
                y=[src_y, dst_y],
                mode='lines+text',
                name=trip['time'],
                line=dict(color='red' if ctg==1 else 'green', 
                          width=2),
            )
            fig.add_trace(trace)
            arrow_annotation = go.layout.Annotation(
                x=dst_x,
                y=dst_y,
                ax=(src_x+dst_x)/2,
                ay=(src_y+dst_y)/2,
                xref='x',
                yref='y',
                axref='x',
                ayref='y',
                showarrow=True,
                arrowhead=2,
                arrowsize=1,
                arrowwidth=2,
                arrowcolor='red' if ctg==1 else 'green',
                text=trip['src']+'=>'+trip['dst'],
                font=dict(size=16),
            )
            fig.add_annotation(arrow_annotation)

    for index, row in routes.iterrows():
        src_type = row['src'][0]
        dst_type = row['dst'][0]
        x_coords = [position.loc[position['station'] == row['src']]['position_x'].iloc[0],
                    position.loc[position['station'] == row['dst']]['position_x'].iloc[0]]
        y_coords = [position.loc[position['station'] == row['src']]['position_y'].iloc[0],
                    position.loc[position['station'] == row['dst']]['position_y'].iloc[0]]
    
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
            hoverinfo='none',
            showlegend=False,
            opacity=0.2,
        )) 

    fig.update_traces(marker=dict(size=10))
    fig.update_layout(
                    width=1000,
                    height=800) 

    return fig

@app.callback(
    Output('check_pacakge', 'children'),
    Input('package-id-dropdown', 'value'),
)

def update_check_package(selected_package_id):
    trips = []
    for index, row in package_trip.iterrows():
        if row['package_id'].strip() == selected_package_id and row['src'] != row['dst']:
            trips.append("At "+ str(row['time']) +" from "+row['src']+" to "+row['dst']+".")
    return f"You selected Package {selected_package_id}, the trip is: \n{''.join(trips)}"

if __name__ == '__main__':
    app.run_server(debug=False, port=8070)

