async function getParameters() {
    const response = await fetch('/api/parameters', { signal: AbortSignal.timeout(3000) });
    if(response.ok) {
    const parameters = await response.json();
    return parameters;
    } else {
        showNotification("Error","Parameter API error " + response.status);
        return {};
    }
}

async function sendCommand(command, temperature) {
    let url = `/api/command/${command}`;
    if(temperature) {
        url += `?temperature=${temperature}`;
    }
    const response = await fetch(url, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            command: command,
            temperature: temperature
        }),
        signal: AbortSignal.timeout(3000)
    });
    if(response.ok) {
        const parameters = await response.json();
        updateUi(parameters);
    } else {
        showNotification("Error","Command API error " + response.status);
    }
}

function updateUi(parameters) {
    document.getElementById("temperature").innerHTML = parameters.externalTemperature ?? "N/A";
    document.getElementById("temperature").className = !parameters.externalTemperature || parameters.externalTemperature > 25 || parameters.externalTemperature < 10 ? "warning" : "";
    document.getElementById("internal-temperature").innerHTML = parameters.internalTemperature  ?? "N/A";
    document.getElementById("internal-temperature").className = !parameters.internalTemperature || parameters.internalTemperature > 60 ? "warning" : "";
    document.getElementById("wifi-signal").innerHTML = parameters.rssi ?? "N/A";
    document.getElementById("wifi-signal").className = !parameters.rssi || parameters.rssi < -60 ? "warning" : "";
    document.getElementById("last-command").innerHTML = parameters.lastIrCommand ?? "N/A";
    document.getElementById("last-command").className = !parameters.lastIrCommand ? "warning" : "";
}

const refreshData = async () => {
    const parameters = await getParameters();
    updateUi(parameters);
}

const showNotification = (title, text, timeout = 2000) => {
    document.getElementById("notification").innerHTML = "<h3>" + title + "</h3><p>" + text + "</p>";
    document.getElementById("notification").className = "show";
    setTimeout(hideNotification, timeout);
}

const hideNotification = () => {
    document.getElementById("notification").className = "";
}

var intervalId = window.setInterval(refreshData, 5000);
refreshData();

document.getElementById("button-off").addEventListener("click", () => sendCommand("off"));
document.getElementById("button-heat-22").addEventListener("click", () => sendCommand("heat",22));
document.getElementById("button-heat-10").addEventListener("click", () => sendCommand("isave",10));