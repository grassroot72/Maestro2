
function login() {
  // Creating a XHR object
  let xhr = new XMLHttpRequest();
  let url = "login.json";

  // Create a state change callback
  xhr.onreadystatechange = function () {
    if (xhr.readyState === 4 && xhr.status === 200) {
      // Print received data from server
      document.getElementById("content").innerHTML=this.responseText;
    }
  };

  let name = document.querySelector('#username');
  let pass = document.querySelector('#password');
  let obj = {"Auth":name.value + "=" + btoa(pass.value)};

  // Converting JSON data to string
  let data = JSON.stringify(obj);
  // open a connection
  xhr.open("POST", url, true);
  // Set the request header i.e. which type of content you are sending
  xhr.setRequestHeader("Content-Type", "application/json");
  // Sending data with the request
  xhr.send(data);
}
