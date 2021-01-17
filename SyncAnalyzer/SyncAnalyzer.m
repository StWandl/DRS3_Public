% Sajan Cherukad, Stefan Wandl, Dominic Zopf
% DRS3 - ESD

clear all;
clc;

% load data
%load 16_ch_testdata.mat
load drs_test_cpp.mat

%% Measurement parameters 
Fs = digital_sample_rate_hz; % Sample frequency [Hz]
Ts = 1 / Fs;
Channels = digital_channel_indexes;
NumChannels = length(Channels);
Init_Bitstates = digital_channel_initial_bitstates;
NumSamples = num_samples_digital;
Duration = 1/Fs * (NumSamples-1);
t_vec = (0:Ts:Duration);

% check for valid number of channels
if mod(NumChannels, 4) ~= 0
    error('Invalid number of channels! Each node should have 4 channels!');
end

NumNodes = NumChannels / 4;
NodeNames = [];

for p=1:NumNodes
    NodeNames = [NodeNames ("Node " + num2str(p))];
end

% check for valid starting point for systick channels
check = find(Init_Bitstates(1:4:end));
if length(check) ~= NumNodes && ~isempty(check)
    error('Invalid starting bitstate of channels! Each systick channel should start at same bitstate!');
end

%% fix input hack for rust implementation
%digital_channel_0(2) = digital_channel_0(2) + digital_channel_0(1);
%digital_channel_0 = digital_channel_0(2:end);

%digital_channle_4 = digital_channel_4(1:end-1);

%% Convert each channel to vectors of 0s and 1s

channel_values = zeros(NumChannels, NumSamples); % holds data
MaxChLength = 0;

for i=1:length(Channels)
   
    ch = Channels(i);
    
    switch ch
        case 0
            values = digital_channel_0;
        case 1
            values = digital_channel_1;
        case 2
            values = digital_channel_2;
        case 3
            values = digital_channel_3;
        case 4
            values = digital_channel_4;
        case 5
            values = digital_channel_5;
        case 6
            values = digital_channel_6;
        case 7
            values = digital_channel_7;
        case 8
            values = digital_channel_8;
        case 9
            values = digital_channel_9;
        case 10
            values = digital_channel_10;
        case 11
            values = digital_channel_11;
        case 12
            values = digital_channel_12;
        case 13
            values = digital_channel_13;
        case 14
            values = digital_channel_14;
        case 15
            values = digital_channel_15;
        otherwise
            error('Too many channels, can only handle 16!');            
    end
    
    if length(values) > MaxChLength
        MaxChLength = length(values);
    end
    
    idx = 1;
    bit = Init_Bitstates(i);
    
    for k = 1:length(values)
       
        val = values(k);
        if bit == 0
            channel_values(i,idx:idx+val-1) = zeros(1,val);
        else
            channel_values(i,idx:idx+val-1) = ones(1,val);
        end
        
        idx = idx + val;
        bit = ~bit;
        
    end
    
end

%% Calculate deviation of each node to the master node

% to do

edge_indices = zeros(NumNodes, MaxChLength-1);

% find indices of all edges
for i = 1:NumNodes
   
    if i == 1
        edges = zeros(1, length(digital_channel_0)-1);
    elseif i == 2
        edges = zeros(1, length(digital_channel_4)-1);
    elseif i == 3
        edges = zeros(1, length(digital_channel_8)-1);
    elseif i == 4
        edges = zeros(1, length(digital_channel_12)-1);
    else
        error('Too many nodes detected! Aborting...');
    end
    
    chIndex = (i-1)*4+1;
    
    nextEdgeBit = ~channel_values(chIndex,1);
    startIdx = 1;
    count = 1;
    
    edge = find(channel_values(chIndex,startIdx:end) == nextEdgeBit, 1, 'first');   
    
    while length(edge) == 1
       edges(count) = edge + startIdx - 1;
       startIdx = startIdx + edge;
       count = count + 1;
       nextEdgeBit = ~nextEdgeBit;
       edge = find(channel_values(chIndex,startIdx:end) == nextEdgeBit, 1, 'first'); 
    end
    
    edge_indices(i,1:length(edges)) = edges;
        
end

deviations = zeros(NumNodes, MaxChLength-1);

% calculate deviation to master for each edge
for j = 1:NumNodes
   
%      vec_length = find(edge_indices(j,:), 1, 'last');
%      
%      if isempty(vec_length)
%          vec_length = MaxChLength;
%      end
     
     for k = 1:MaxChLength-1%vec_length
          
        if edge_indices(j,k) == 0
            if k > 1
                deviation_t = deviations(j,k-1);
            else
                deviation_t = 0;
            end
        else
            curIdx = edge_indices(j,k);
            curMaster = find(channel_values(3:4:end,curIdx), 1, 'first');
         
            if length(curMaster) == 0
                deviation_t = 0;  % no master at that particular time
            else
                deviation_nSamples = edge_indices(j,k) - edge_indices(curMaster,k);
                deviation_t = deviation_nSamples * Ts;
            end
        end       
        deviations(j,k) = deviation_t;         
     end    
end

% map deviatons to time vector
deviations_vec = zeros(NumNodes, NumSamples);

for i=1:NumNodes
   
    for j=1:MaxChLength-1
       
        startIdx = edge_indices(i,j);
        
        if startIdx ~= 0
            if j < MaxChLength-1         
                endIdx = edge_indices(i,j+1);
                if endIdx == 0
                    count = NumSamples - startIdx;
                else
                    count = endIdx - startIdx;
                end
            else
                count = NumSamples - startIdx;
            end        

            deviations_vec(i, startIdx+1:startIdx+count) = deviations(i,j);
        end       
    end
    
end

%% Plot signals

figure(1);
clf;
hold on;
plot(t_vec, channel_values(1:4:end,:), 'Linewidth', 2);
title('SysTick');
ylim([-1.5 2]);
xlabel('t [s]');
legend(NodeNames);

%%

figure(2);
clf;
hold on;
plot(t_vec, channel_values(2:4:end,:));
title({'Communication', ...
    ['Master: Sending = rising edge, Receiving = falling edge']...
    ['Slave: Sending = falling edge, Receiving = rising edge']});
ylim([-1.5 2]);
xlabel('t [s]');
legend(NodeNames);

figure(3);
clf;
hold on;
plot(t_vec, channel_values(3:4:end,:));
title('Master(1) or Slave (0)');
ylim([-1.5 2]);
xlabel('t [s]');
legend(NodeNames);

figure(4);
clf;
hold on;
plot(t_vec, channel_values(4:4:end,:));
title({'Connected to other nodes', ...
    ['Master: 1 if a minimum of 1 slave is connected, else 0']...
    ['Slave: 1 if connected to master, else 0']});
ylim([-1.5 2]);
xlabel('t [s]');
legend(NodeNames);

%% Plot deviations

figure(5)
clf;
hold on;
plot(t_vec,deviations_vec, 'Linewidth', 2);
title('Deviation to master');
xlabel('t [s]');
ylabel('deviation [s]');
legend(NodeNames);
